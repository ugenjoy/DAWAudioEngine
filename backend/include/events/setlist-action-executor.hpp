#pragma once

#include <memory>
#include <vector>
#include "events/action-executor.hpp"

/**
 * Returns executors for setlist.next, setlist.prev.
 */
std::vector<std::unique_ptr<ActionExecutor>> createSetlistActionExecutors();
