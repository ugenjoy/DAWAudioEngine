#pragma once

#include <memory>
#include "events/action-executor.hpp"

/** Create a MidiActionExecutor instance (handles "midi.send" actions). */
std::unique_ptr<ActionExecutor> createMidiActionExecutor();
