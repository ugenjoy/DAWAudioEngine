#pragma once

#include <crow.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

/**
 * WebSocketServer - Simple WebSocket server using Crow
 *
 * This class encapsulates a Crow HTTP/WebSocket server that runs on a separate
 * thread. For now, it just accepts connections and logs events - no business
 * logic yet.
 */
class WebSocketServer {
 public:
  WebSocketServer() : running_(false), thread_exited_(false) {}

  ~WebSocketServer() { stop(); }

  /**
   * Start the WebSocket server on the specified port
   * The server runs on a separate thread to not block the audio engine
   */
  void start(uint16_t port = 8080) {
    if (running_.load()) {
      std::cerr << "[WebSocket] Server already running" << std::endl;
      return;
    }

    port_ = port;
    running_.store(true);

    // Launch server on separate thread
    server_thread_ = std::thread([this]() { this->run(); });

    std::cout << "[WebSocket] Server starting on port " << port_ << std::endl;
  }

  /**
   * Stop the WebSocket server gracefully
   */
  void stop() {
    if (!running_.load() && !server_thread_.joinable()) {
      return;
    }

    std::cout << "[WebSocket] Stopping server..." << std::endl;

    if (app_ && running_.load()) {
      try {
        app_->stop();
      } catch (...) {
        // Ignore exceptions during shutdown
      }
    }

    if (server_thread_.joinable()) {
      try {
        server_thread_.join();
      } catch (...) {
        // Ignore exceptions during thread join
      }
    }

    running_.store(false);
    std::cout << "[WebSocket] Server stopped" << std::endl;
  }

  bool isRunning() const { return running_.load(); }

  // Check if server thread has exited (e.g., due to Ctrl+C)
  bool hasExited() const { return thread_exited_.load(); }

 private:
  void run() {
    app_ = std::make_unique<crow::SimpleApp>();

    // WebSocket endpoint
    CROW_WEBSOCKET_ROUTE((*app_), "/ws")
        .onopen([](crow::websocket::connection& conn) {
          std::cout << "[WebSocket] Client connected" << std::endl;
        })
        .onclose(
            [](crow::websocket::connection& conn, const std::string& reason) {
              std::cout << "[WebSocket] Client disconnected: " << reason
                        << std::endl;
            })
        .onmessage([](crow::websocket::connection& conn,
                      const std::string& data, bool is_binary) {
          std::cout << "[WebSocket] Received message: " << data << std::endl;
          // Echo back for now
          conn.send_text("Echo: " + data);
        })
        .onerror(
            [](crow::websocket::connection& conn, const std::string& error) {
              std::cerr << "[WebSocket] Error: " << error << std::endl;
            });

    // Simple HTTP endpoint for health check
    CROW_ROUTE((*app_), "/health")
    ([]() { return crow::response(200, "OK"); });

    // Run the server (blocking call)
    app_->port(port_).multithreaded().run();

    thread_exited_.store(true);
    std::cout << "[WebSocket] Server thread exited" << std::endl;
  }

  std::unique_ptr<crow::SimpleApp> app_;
  std::thread server_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> thread_exited_;
  uint16_t port_;
};
