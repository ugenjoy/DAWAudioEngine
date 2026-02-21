#pragma once

#include <juce_core/juce_core.h>
#include "commands/command-queue.hpp"

// Forward declaration
class AppContext;

/**
 * Worker thread that processes commands from the queue.
 * Runs in its own thread, continuously polling the queue and executing
 * commands.
 *
 * This isolates the WebSocket thread from the audio engine, ensuring
 * thread-safe communication without blocking.
 */
class CommandProcessor : public juce::Thread {
 public:
  /**
   * Create a command processor.
   * @param queue The command queue to process
   * @param ctx Application context providing access to all services
   */
  CommandProcessor(CommandQueue& queue, AppContext& ctx);

  ~CommandProcessor() override;

  /**
   * Start processing commands.
   */
  void startProcessing();

  /**
   * Stop processing commands (waits for thread to finish).
   */
  void stopProcessing();

 private:
  void run() override;

  CommandQueue& queue;
  AppContext& ctx;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandProcessor)
};
