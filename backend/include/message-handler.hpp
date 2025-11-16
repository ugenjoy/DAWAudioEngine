#pragma once

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include "audio-engine-core.hpp"

using json = nlohmann::json;

class MessageHandler {
 public:
  MessageHandler(std::shared_ptr<AudioEngineCore> engine)
      : audioEngine(engine) {}

  json handleMessage(const std::string& rawMessage) {
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
        // if (action == "toggle")
        //   return handleToggle();
        // if (action == "setVolume")
        //   return handleSetVolume(message);
        // if (action == "setTempo")
        //   return handleSetTempo(message);
        // if (action == "getState")
        //   return handleGetState();

        return errorResponse("Unknown action: " + action);
      }

      return errorResponse("Unknown type: " + type);

    } catch (const json::exception& e) {
      return errorResponse("Invalid JSON: " + std::string(e.what()));
    }
  }

 private:
  std::shared_ptr<AudioEngineCore> audioEngine;

  json handlePlay() {
    audioEngine->play();
    return {{"status", "ok"}, {"playing", true}};
  }

  json handlePause() {
    audioEngine->pause();
    return {{"status", "ok"}, {"playing", false}};
  }

  json handleStop() {
    audioEngine->stop();
    return {{"status", "ok"}, {"playing", false}};
  }

  // json handleToggle() {
  //   audioEngine->togglePlayback();
  //   return {{"status", "ok"}, {"playing", audioEngine->isPlaying()}};
  // }

  // json handleSetVolume(const json& message) {
  //   if (!message.contains("value")) {
  //     return errorResponse("Missing 'value' parameter");
  //   }

  //   float volume = message["value"];
  //   audioEngine->setMasterVolume(volume);
  //   return {{"status", "ok"}, {"volume", volume}};
  // }

  // json handleSetTempo(const json& message) {
  //   if (!message.contains("value")) {
  //     return errorResponse("Missing 'value' parameter");
  //   }

  //   float tempo = message["value"];
  //   audioEngine->setTempo(tempo);
  //   return {{"status", "ok"}, {"tempo", tempo}};
  // }

  // json handleGetState() {
  //   return {{"status", "ok"},
  //           {"playing", audioEngine->isPlaying()},
  //           {"volume", audioEngine->getMasterVolume()},
  //           {"tempo", audioEngine->getTempo()}};
  // }

  json errorResponse(const std::string& message) {
    return {{"status", "error"}, {"message", message}};
  }
};