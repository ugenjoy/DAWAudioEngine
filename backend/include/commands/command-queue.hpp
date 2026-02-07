#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include "commands/command.hpp"

/**
 * Lock-free command queue for thread-safe communication.
 * Uses JUCE's AbstractFifo for lock-free SPSC (Single Producer Single Consumer)
 * operations.
 *
 * Thread safety:
 * - push() can be called from WebSocket thread
 * - pop() can be called from Worker thread
 * - No locks, no blocking, safe for real-time contexts
 */
class CommandQueue {
 public:
  /**
   * Create a command queue with specified capacity.
   * @param capacity Maximum number of commands that can be queued
   */
  explicit CommandQueue(int capacity = 1024);

  ~CommandQueue() = default;

  /**
   * Push a command onto the queue (non-blocking).
   * @param command The command to enqueue
   * @return true if successful, false if queue is full
   */
  bool push(CommandPtr command);

  /**
   * Pop a command from the queue (non-blocking).
   * @return The command, or nullptr if queue is empty
   */
  CommandPtr pop();

  /**
   * Get the number of commands currently in the queue.
   */
  int getNumReady() const { return fifo.getNumReady(); }

  /**
   * Check if the queue is empty.
   */
  bool isEmpty() const { return fifo.getNumReady() == 0; }

 private:
  juce::AbstractFifo fifo;
  std::vector<CommandPtr> buffer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandQueue)
};
