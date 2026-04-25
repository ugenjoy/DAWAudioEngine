#pragma once

#include <memory>
#include <vector>
#include "events/action-executor.hpp"

/**
 * Returns executors for transport.play, transport.pause, transport.stop, transport.seekToPosition.
 */
std::vector<std::unique_ptr<ActionExecutor>> createTransportActionExecutors();
