#pragma once

#include <memory>
#include <vector>
#include "events/action-executor.hpp"

/**
 * Returns executors for loop.cancel, loop.exit.
 */
std::vector<std::unique_ptr<ActionExecutor>> createLoopActionExecutors();
