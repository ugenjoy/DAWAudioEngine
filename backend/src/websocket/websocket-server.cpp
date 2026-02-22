#include "websocket/websocket-server.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

WebSocketServer::WebSocketServer(CommandQueue& commandQueue,
                                 const CommandFactory& commandFactory,
                                 const ModeManager& modeManager,
                                 int port)
    : commandQueue(commandQueue),
      commandFactory(commandFactory),
      modeManager(modeManager),
      port(port) {}

WebSocketServer::~WebSocketServer() { stop(); }

void WebSocketServer::startAsync() {
  if (running.load()) return;

  serverThread = std::thread([this]() { run(); });

  juce::Logger::writeToLog("[WebSocket] Server starting on port " +
                           juce::String(port));
}

void WebSocketServer::run() {
  juce::Logger::writeToLog("[WebSocket] Server thread started");

  app = std::make_unique<crow::SimpleApp>();

  // WebSocket endpoint
  CROW_WEBSOCKET_ROUTE((*app), "/ws")
      .onopen([this](crow::websocket::connection& conn) {
        {
          std::lock_guard<std::mutex> lock(clientsMutex);
          clients.insert(&conn);
        }
        juce::Logger::writeToLog("[WebSocket] Client connected (total: " +
                                 juce::String((int)clients.size()) + ")");
        json welcome = {{"type", "connected"},
                        {"message", "Connected to DAW Audio Engine"}};
        conn.send_text(welcome.dump());
      })
      .onclose(
          [this](crow::websocket::connection& conn, const std::string& reason) {
            {
              std::lock_guard<std::mutex> lock(clientsMutex);
              clients.erase(&conn);
            }
            juce::Logger::writeToLog(
                "[WebSocket] Client disconnected: " + juce::String(reason) +
                " (total: " + juce::String((int)clients.size()) + ")");
          })
      .onmessage([this](crow::websocket::connection& conn,
                        const std::string& data, bool /*is_binary*/) {
        juce::Logger::writeToLog("[WebSocket] Received: " + juce::String(data));

        try {
          auto msg = json::parse(data);
          std::string action = msg.value("action", "");

          // Reject edit-only commands in Live mode
          if (modeManager.isLiveMode() && commandFactory.isEditOnly(action)) {
            json error = {{"type", "error"},
                          {"code", "edit_only"},
                          {"message", "Action '" + action +
                                          "' is not available in Live mode"}};
            conn.send_text(error.dump());
            return;
          }

          // Use factory to create command
          CommandPtr cmd = commandFactory.create(action, msg);

          if (cmd) {
            // Attach a reply callback so the command can respond to this
            // specific client without broadcasting to all connected clients.
            // Thread-safe: clientsMutex guards both the clients set and sends,
            // so a disconnect between push() and execute() is handled cleanly.
            cmd->setReply([this, &conn](const std::string& response) {
              std::lock_guard<std::mutex> lock(clientsMutex);
              if (clients.count(&conn) > 0) {
                try {
                  conn.send_text(response);
                } catch (const std::exception& e) {
                  juce::Logger::writeToLog(
                      "[WebSocket] Error sending reply: " +
                      juce::String(e.what()));
                }
              }
            });

            if (commandQueue.push(std::move(cmd))) {
              json response = {
                  {"type", "ack"}, {"action", action}, {"status", "queued"}};
              conn.send_text(response.dump());
            } else {
              json error = {{"type", "error"},
                            {"message", "Command queue is full"}};
              conn.send_text(error.dump());
            }
          } else {
            json error = {{"type", "error"},
                          {"message", "Unknown action: " + action}};
            conn.send_text(error.dump());
          }

        } catch (const std::exception& e) {
          json error = {{"type", "error"}, {"message", e.what()}};
          conn.send_text(error.dump());
        }
      })
      .onerror([](crow::websocket::connection& /*conn*/,
                  const std::string& error) {
        juce::Logger::writeToLog("[WebSocket] Error: " + juce::String(error));
      });

  // Simple HTTP endpoint for health check
  CROW_ROUTE((*app), "/health")([]() { return crow::response(200, "OK"); });

  running.store(true);
  juce::Logger::writeToLog("[WebSocket] Server listening on port " +
                           juce::String(port));

  // Run the server (blocking call)
  app->port(port).multithreaded().run();

  // If we get here, the server has stopped
  threadExited.store(true);
  juce::Logger::writeToLog("[WebSocket] Server thread exited");
}

void WebSocketServer::stop() {
  if (!running.load() && !serverThread.joinable()) return;

  juce::Logger::writeToLog("[WebSocket] Stopping server");

  if (app) {
    app->stop();
  }

  if (serverThread.joinable()) {
    serverThread.join();
  }

  running.store(false);
}

void WebSocketServer::broadcast(const std::string& message) {
  std::lock_guard<std::mutex> lock(clientsMutex);

  // juce::Logger::writeToLog("[WebSocket] Broadcasting to " +
  //                          juce::String((int)clients.size()) + " client(s)");

  for (auto* client : clients) {
    try {
      client->send_text(message);
    } catch (const std::exception& e) {
      juce::Logger::writeToLog("[WebSocket] Error broadcasting: " +
                               juce::String(e.what()));
    }
  }
}

