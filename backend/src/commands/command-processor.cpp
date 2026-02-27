#include "commands/command-processor.hpp"

#include "app-context.hpp"

CommandProcessor::CommandProcessor(CommandQueue& queue, AppContext& ctx)
    : Thread("CommandProcessor"), queue(queue), ctx(ctx) {}

CommandProcessor::~CommandProcessor() { stopProcessing(); }

void CommandProcessor::startProcessing() { startThread(); }

void CommandProcessor::setPollInterval(int ms) {
  pollIntervalMs.store(ms);
}

void CommandProcessor::stopProcessing() {
  signalThreadShouldExit();
  notify();          // Wake up the thread if it's waiting
  stopThread(1000);  // Wait up to 1 second for thread to finish
}

void CommandProcessor::run() {
  while (!threadShouldExit()) {
    if (CommandPtr cmd = queue.pop()) {
      try {
        DBG("[CommandProcessor] Executing: " << cmd->getName());
        cmd->execute(ctx);
      } catch (const std::exception& e) {
        DBG("[CommandProcessor] Error executing " << cmd->getName() << ": "
                                                  << e.what());
      }
    } else {
      // Queue empty, wait before checking again
      // Live mode: 10ms (fewer commands), Edit mode: 1ms (lower latency)
      wait(pollIntervalMs.load());
    }
  }
}
