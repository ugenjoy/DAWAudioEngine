#include "model/project.hpp"

#include <cassert>
#include <juce_core/juce_core.h>

// ---------------------------------------------------------------------------
// Factories
// ---------------------------------------------------------------------------

std::unique_ptr<Project> Project::createNew(std::string name,
                                            std::string parentDir) {
  auto p = std::unique_ptr<Project>(new Project());
  p->name = std::move(name);
  p->id = juce::Uuid().toString().toStdString();

  // Build the full project path: <parentDir>/<name>.dawproj
  juce::File parentFolder(parentDir);
  juce::File projectFolder =
      parentFolder.getChildFile(p->name + ".dawproj");
  p->path = projectFolder.getFullPathName().toStdString();

  // Create directory structure
  auto createDir = [&](const juce::File& dir) -> bool {
    if (!dir.exists()) {
      juce::Result result = dir.createDirectory();
      if (result.failed()) {
        p->lastError = "Failed to create directory " +
                       dir.getFullPathName().toStdString() + ": " +
                       result.getErrorMessage().toStdString();
        return false;
      }
    }
    return true;
  };

  if (!createDir(projectFolder)) return nullptr;
  if (!createDir(projectFolder.getChildFile("audio"))) return nullptr;
  if (!createDir(projectFolder.getChildFile("cache"))) return nullptr;

  // Initialize managers
  p->songsManager = std::make_unique<SongsManager>();
  p->setlistManager = std::make_unique<SetlistManager>();

  // Persist the initial project.json
  if (!p->save()) return nullptr;

  return p;
}

std::unique_ptr<Project> Project::loadFromDisk(std::string path) {
  auto p = std::unique_ptr<Project>(new Project());

  juce::File projectFolder(path);
  if (!projectFolder.exists() || !projectFolder.isDirectory()) {
    p->lastError = "Project folder does not exist or is not a directory";
    return nullptr;
  }

  juce::File projectFile = projectFolder.getChildFile("project.json");
  if (!projectFile.exists()) {
    p->lastError = "project.json file not found in project folder";
    return nullptr;
  }

  try {
    std::string jsonString = projectFile.loadFileAsString().toStdString();
    nlohmann::json projectJson = nlohmann::json::parse(jsonString);

    // Read identity fields from JSON
    p->id = projectJson.value("id", "");
    p->name = projectJson.value("name", "");

    // Fall back to a freshly generated UUID for legacy files missing an id
    if (p->id.empty()) {
      p->id = juce::Uuid().toString().toStdString();
    }

    // path and lastModified come from the filesystem
    p->path = projectFolder.getFullPathName().toStdString();
    p->lastModified =
        projectFile.getLastModificationTime().toISO8601(true).toStdString();

    // Derive name from folder if not set (legacy projects)
    if (p->name.empty()) {
      p->name = projectFolder.getFileNameWithoutExtension().toStdString();
    }

    // Initialize and populate managers
    p->songsManager = std::make_unique<SongsManager>();
    p->setlistManager = std::make_unique<SetlistManager>();

    if (projectJson.contains("songs")) {
      p->songsManager->loadFromJson(projectJson["songs"]);
    }

    if (projectJson.contains("events")) {
      p->songsManager->loadProjectEventsFromJson(projectJson["events"]);
    }

    if (projectJson.contains("setlists")) {
      p->setlistManager->loadFromJson(projectJson["setlists"]);
    }

  } catch (const nlohmann::json::parse_error& e) {
    p->lastError = std::string("JSON parse error: ") + e.what();
    return nullptr;
  } catch (const std::exception& e) {
    p->lastError = std::string("Error loading project: ") + e.what();
    return nullptr;
  }

  return p;
}

Project Project::readMetadata(std::string path) {
  Project p;

  juce::File dir(path);
  p.path = dir.getFullPathName().toStdString();
  const std::string folderName = dir.getFileNameWithoutExtension().toStdString();

  juce::File projectFile = dir.getChildFile("project.json");
  if (!projectFile.existsAsFile()) {
    p.name = folderName;
    return p;
  }

  try {
    std::string jsonString = projectFile.loadFileAsString().toStdString();
    nlohmann::json projectJson = nlohmann::json::parse(jsonString);

    p.id = projectJson.value("id", "");
    p.name = projectJson.value("name", "");
  } catch (...) {
    // Malformed JSON — fall back to folder name
  }

  if (p.id.empty()) {
    p.id = juce::Uuid().toString().toStdString();
  }
  if (p.name.empty()) {
    p.name = folderName;
  }

  p.lastModified =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

  return p;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

bool Project::save() {
  lastError = "";

  juce::File projectFolder(path);

  // Ensure the path ends with .dawproj
  if (!projectFolder.getFileName().endsWith(".dawproj")) {
    lastError = "Project path must end with .dawproj extension";
    return false;
  }

  // Ensure directory structure exists
  auto createDir = [&](const juce::File& dir) -> bool {
    if (!dir.exists()) {
      juce::Result result = dir.createDirectory();
      if (result.failed()) {
        lastError = "Failed to create directory " +
                    dir.getFullPathName().toStdString() + ": " +
                    result.getErrorMessage().toStdString();
        return false;
      }
    }
    return true;
  };

  if (!createDir(projectFolder)) return false;
  if (!createDir(projectFolder.getChildFile("audio"))) return false;
  if (!createDir(projectFolder.getChildFile("cache"))) return false;

  // Build JSON
  nlohmann::json projectJson;
  projectJson["id"] = id;
  projectJson["name"] = name;
  projectJson["version"] = "1.0.0";

  if (songsManager) {
    projectJson["songs"] = songsManager->toJson();
    projectJson["events"] = songsManager->projectEventsToJson();
  } else {
    projectJson["songs"] = nlohmann::json::array();
    projectJson["events"] = nlohmann::json::array();
  }

  if (setlistManager) {
    projectJson["setlists"] = setlistManager->toJson();
  } else {
    projectJson["setlists"] = nlohmann::json::array();
  }

  // path and lastModified are filesystem concerns — don't persist them so the
  // folder stays portable across machines
  // (they are intentionally omitted from projectJson)

  // Strip waveform data from clips (regenerated on load from audio files)
  if (projectJson.contains("songs")) {
    for (auto& song : projectJson["songs"]) {
      if (song.contains("tracks")) {
        for (auto& track : song["tracks"]) {
          if (track.contains("clips")) {
            for (auto& clip : track["clips"]) {
              clip.erase("waveform");
            }
          }
        }
      }
    }
  }

  // Write JSON to project.json
  juce::File projectFile = projectFolder.getChildFile("project.json");
  try {
    std::string jsonString = projectJson.dump(2);
    if (!projectFile.replaceWithText(jsonString)) {
      lastError = "Failed to write project.json file";
      return false;
    }
  } catch (const std::exception& e) {
    lastError = std::string("JSON serialization error: ") + e.what();
    return false;
  }

  // Refresh lastModified from the freshly written file
  lastModified =
      projectFile.getLastModificationTime().toISO8601(true).toStdString();

  return true;
}

const std::string& Project::getLastError() const { return lastError; }

// ---------------------------------------------------------------------------
// Identity accessors
// ---------------------------------------------------------------------------

const std::string& Project::getId() const { return id; }
const std::string& Project::getName() const { return name; }
const std::string& Project::getPath() const { return path; }
const std::string& Project::getLastModified() const { return lastModified; }

std::string Project::getAudioDir() const {
  return juce::File(path).getChildFile("audio").getFullPathName().toStdString();
}

// ---------------------------------------------------------------------------
// Contents accessors
// ---------------------------------------------------------------------------

SongsManager& Project::songs() {
  assert(songsManager && "songs() called on a readMetadata snapshot");
  return *songsManager;
}
const SongsManager& Project::songs() const {
  assert(songsManager && "songs() called on a readMetadata snapshot");
  return *songsManager;
}
SetlistManager& Project::setlists() {
  assert(setlistManager && "setlists() called on a readMetadata snapshot");
  return *setlistManager;
}
const SetlistManager& Project::setlists() const {
  assert(setlistManager && "setlists() called on a readMetadata snapshot");
  return *setlistManager;
}

const std::vector<EventRule>& Project::getEventRules() const {
  assert(songsManager && "getEventRules() called on a readMetadata snapshot");
  return songsManager->getProjectEventRules();
}

void Project::setEventRules(std::vector<EventRule> rules) {
  assert(songsManager && "setEventRules() called on a readMetadata snapshot");
  songsManager->setProjectEventRules(std::move(rules));
}

// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------

nlohmann::json Project::toJson() const {
  nlohmann::json j;
  j["id"] = id;
  j["name"] = name;
  j["path"] = path;
  j["lastModified"] = lastModified;

  if (songsManager) {
    j["songs"] = songsManager->toJson();
    j["setlists"] = setlistManager ? setlistManager->toJson()
                                   : nlohmann::json::array();
    j["events"] = songsManager->projectEventsToJson();
  }
  // If songsManager is null (readMetadata case), omit contents — identity
  // fields above are sufficient.

  return j;
}
