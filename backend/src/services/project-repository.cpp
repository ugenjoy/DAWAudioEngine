#include "services/project-repository.hpp"

#include <juce_core/juce_core.h>

namespace ProjectRepository {

std::string getDefaultProjectsDirectory() {
  juce::File home =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory);
  return home.getChildFile("daw/projects").getFullPathName().toStdString();
}

std::vector<Project> scan(const std::string& directory) {
  std::vector<Project> projects;

  juce::File dir(directory);
  if (!dir.exists() || !dir.isDirectory()) {
    return projects;
  }

  auto children =
      dir.findChildFiles(juce::File::findDirectories, false, "*.dawproj");

  projects.reserve(static_cast<size_t>(children.size()));
  for (const auto& child : children) {
    projects.push_back(
        Project::readMetadata(child.getFullPathName().toStdString()));
  }

  return projects;
}

}  // namespace ProjectRepository
