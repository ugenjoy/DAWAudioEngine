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
    const std::string& deviceIdentifier) {
  auto it = openDevices.find(deviceIdentifier);
  if (it != openDevices.end()) {
    return it->second.get();
  }

  // Try to open the device
  auto device = juce::MidiOutput::openDevice(
      juce::String(deviceIdentifier));

  if (!device) {
    // Also try by name if identifier lookup failed
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& info : devices) {
      if (info.name.toStdString() == deviceIdentifier) {
        device = juce::MidiOutput::openDevice(info.identifier);
        break;
      }
    }
  }

  if (!device) return nullptr;

  auto* ptr = device.get();
  openDevices[deviceIdentifier] = std::move(device);
  return ptr;
}
