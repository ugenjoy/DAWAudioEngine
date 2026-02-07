#include "commands/command-processor.hpp"
#include "app-context.hpp"

CommandProcessor::CommandProcessor(CommandQueue& queue, AppContext& ctx)
    : Thread("CommandProcessor"), queue(queue), ctx(ctx) {}

CommandProcessor::~CommandProcessor() { stopProcessing(); }

void CommandProcessor::startProcessing() { startThread(); }

void CommandProcessor::stopProcessing() {
  signalThreadShouldExit();
  notify();  // Wake up the thread if it's waiting
  stopThread(1000);  // Wait up to 1 second for thread to finish
}

void CommandProcessor::run() {
  while (!threadShouldExit()) {
    if (CommandPtr cmd = queue.pop()) {
      try {
        DBG("[CommandProcessor] Executing: " << cmd->getName());
        cmd->execute(ctx);
        commandsProcessed++;
      } catch (const std::exception& e) {
        DBG("[CommandProcessor] Error executing " << cmd->getName() << ": "
                                                  << e.what());
      }
    } else {
      // Queue empty, wait a bit before checking again
      wait(1);  // 1ms sleep to avoid busy-waiting
    }
  }
}
