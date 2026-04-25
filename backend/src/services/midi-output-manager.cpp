#include "services/midi-output-manager.hpp"

nlohmann::json MidiOutputManager::getAvailableOutputs() const {
  nlohmann::json result = nlohmann::json::array();
  auto devices = juce::MidiOutput::getAvailableDevices();

  for (const auto& device : devices) {
    nlohmann::json entry;
    entry["name"] = device.name.toStdString();
    entry["identifier"] = device.identifier.toStdString();
    result.push_back(entry);
  }

  return result;
}

bool MidiOutputManager::sendMessage(const std::string& deviceIdentifier,
                                    const juce::MidiMessage& message) {
  auto* device = getOrOpenDevice(deviceIdentifier);
  if (!device) {
    juce::Logger::writeToLog(
        "[MidiOutputManager] Failed to open device: " +
        juce::String(deviceIdentifier));
    return false;
  }

  device->sendMessageNow(message);
  return true;
}

void MidiOutputManager::closeAll() {
  openDevices.clear();
}

juce::MidiOutput* MidiOutputManager::getOrOpenDevice(
    const std::string& deviceNameOrId) {
  // Resolve the current device info from the OS (by name, then by identifier
  // as a legacy fallback for old saved rules that stored identifiers).
  auto available = juce::MidiOutput::getAvailableDevices();
  juce::MidiDeviceInfo targetInfo;
  bool found = false;

  for (const auto& info : available) {
    if (info.name.toStdString() == deviceNameOrId) {
      targetInfo = info;
      found = true;
      break;
    }
  }
  if (!found) {
    for (const auto& info : available) {
      if (info.identifier.toStdString() == deviceNameOrId) {
        targetInfo = info;
        found = true;
        break;
      }
    }
  }

  if (!found) {
    openDevices.erase(deviceNameOrId);
    return nullptr;
  }

  // Return cached handle if the OS identifier hasn't changed.
  // A changed identifier means the device was reconnected and the old handle
  // is stale — close it and reopen with the new identifier.
  auto it = openDevices.find(deviceNameOrId);
  if (it != openDevices.end()) {
    if (it->second.identifier == targetInfo.identifier.toStdString()) {
      return it->second.output.get();
    }
    openDevices.erase(it);
  }

  auto device = juce::MidiOutput::openDevice(targetInfo.identifier);
  if (!device) return nullptr;

  auto* ptr = device.get();
  openDevices[deviceNameOrId] = {targetInfo.identifier.toStdString(),
                                  std::move(device)};
  return ptr;
}
