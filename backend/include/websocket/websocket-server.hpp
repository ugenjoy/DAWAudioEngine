#pragma once

#include <crow.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include "commands/command-factory.hpp"
#include "commands/command-queue.hpp"
#include "services/mode-manager.hpp"

class AppContext;

/**
 * WebSocket server for client communication.
 * Receives JSON messages, uses CommandFactory to create commands,
 * and pushes them to the queue.
 *
 * The server runs in its own thread and can be started/stopped asynchronously.
 * Supports broadcasting messages to all connected clients.
 */
class WebSocketServer {
 public:
  WebSocketServer(CommandQueue& commandQueue,
                  const CommandFactory& commandFactory,
                  const ModeManager& modeManager,
                  int port = 8080);
  ~WebSocketServer();

  /**
   * Start the server in a background thread.
   * Non-blocking call.
   */
  void startAsync();

  /**
   * Stop the server and wait for the thread to finish.
   * Blocking call.
   */
  void stop();

  /**
   * Broadcast a message to all connected clients.
   * Thread-safe.
   * @param message JSON message to broadcast
   */
  void broadcast(const std::string& message);

  /** Inject AppContext after construction (avoids circular dependency). */
  void setAppContext(AppContext* ctx) { appContext = ctx; }

  bool isRunning() const { return running.load(); }
  int getPort() const { return port; }
  bool hasExited() const { return threadExited.load(); }

 private:
  void run();

  CommandQueue& commandQueue;
  const CommandFactory& commandFactory;
  const ModeManager& modeManager;
  int port;
  std::atomic<bool> running{false};
  std::atomic<bool> threadExited{false};

  std::unique_ptr<crow::SimpleApp> app;
  std::thread serverThread;

  // App context (injected after construction)
  AppContext* appContext = nullptr;

  // Connected clients
  std::mutex clientsMutex;
  std::set<crow::websocket::connection*> clients;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebSocketServer)
};
