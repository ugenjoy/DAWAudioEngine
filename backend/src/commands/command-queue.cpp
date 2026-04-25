#include "commands/command-queue.hpp"

CommandQueue::CommandQueue(int capacity) : fifo(capacity), buffer(capacity) {}

bool CommandQueue::push(CommandPtr command) {
  int start1, size1, start2, size2;
  fifo.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    buffer[start1] = std::move(command);
    fifo.finishedWrite(1);
    return true;
  }

  // Queue is full
  return false;
}

CommandPtr CommandQueue::pop() {
  int start1, size1, start2, size2;
  fifo.prepareToRead(1, start1, size1, start2, size2);

  if (size1 > 0) {
    CommandPtr command = std::move(buffer[start1]);
    fifo.finishedRead(1);
    return command;
  }

  // Queue is empty
  return nullptr;
}
