#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include "audio-engine-core.hpp"
#include "project-manager.hpp"
#include "songs-manager.hpp"

using json = nlohmann::json;

class MessageHandler {
 public:
  explicit MessageHandler(AudioEngineCore* engine, SongsManager* songsManager)
      : audioEngine(engine),
        songsManager(songsManager),
        projectManager(std::make_unique<ProjectManager>()) {}

  json handleMessage(const std::string& rawMessage) const {
    try {
      auto message = json::parse(rawMessage);
      std::string type = message.value("type", "");

      if (type == "action") {
        std::string action = message["payload"]["action"];

        if (action.empty()) {
          return errorResponse("Missing action");
        }

        if (action == "play")
          return handlePlay();
        if (action == "pause")
          return handlePause();
        if (action == "stop")
          return handleStop();
        if (action == "switchSong") {
          int songId = message["payload"].value("songId", -1);
          if (songId < 0) {
            return errorResponse("Missing or invalid songId");
          }
          return handleSwitchSong(songId);
        }
        if (action == "saveProject") {
          std::string path = message["payload"].value("path", "");
          if (path.empty()) {
            return errorResponse("Missing or invalid path");
          }
          return handleSaveProject(path);
        }
        if (action == "loadProject") {
          std::string path = message["payload"].value("path", "");
          if (path.empty()) {
            return errorResponse("Missing or invalid path");
          }
          return handleLoadProject(path);
        }
        if (action == "getSongs") {
          return handleGetSongs();
        }

        return errorResponse("Unknown action: " + action);
      }

      return errorResponse("Unknown type: " + type);

    } catch (const json::exception& e) {
      return errorResponse("Invalid JSON: " + std::string(e.what()));
    }
  }

 private:
  AudioEngineCore* audioEngine;
  SongsManager* songsManager;
  std::unique_ptr<ProjectManager> projectManager;

  json handlePlay() const {
    audioEngine->play();
    return {{"status", "ok"}, {"playing", true}};
  }

  json handlePause() const {
    audioEngine->pause();
    return {{"status", "ok"}, {"playing", false}};
  }

  json handleStop() const {
    audioEngine->stop();
    return {{"status", "ok"}, {"playing", false}};
  }

  json handleSwitchSong(int songId) const {
    auto* song = songsManager->getSong(songId);
    if (!song) {
      return errorResponse("Invalid song ID");
    }

    audioEngine->loadSong(song);
    return {{"status", "ok"}, {"songId", songId}};
  }

  json handleSaveProject(const std::string& path) const {
    if (!projectManager->saveProject(path, *songsManager)) {
      return errorResponse("Failed to save project: " +
                           projectManager->getLastError());
    }
    return {{"status", "ok"},
            {"message", "Project saved successfully"},
            {"path", path}};
  }

  json handleLoadProject(const std::string& path) const {
    if (!projectManager->loadProject(path, *songsManager)) {
      return errorResponse("Failed to load project: " +
                           projectManager->getLastError());
    }
    return {{"status", "ok"},
            {"message", "Project loaded successfully"},
            {"path", path}};
  }

  json handleGetSongs() const {
    return {{"status", "ok"}, {"songs", songsManager->toJson()}};
  }

  json errorResponse(const std::string& message) const {
    return {{"status", "error"}, {"message", message}};
  }
};