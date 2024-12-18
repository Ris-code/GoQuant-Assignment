// WebSocketServer.cpp

#include "WebSocketServer.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp> // Include JSON library
#include <iostream>

// For simplicity, using namespace for JSON
using json = nlohmann::json;

WebSocketServer::WebSocketServer(int port) : port_(port) {
    server_.init_asio();
    server_.set_open_handler(std::bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    server_.set_close_handler(std::bind(&WebSocketServer::on_close, this, std::placeholders::_1));
    server_.set_message_handler(std::bind(&WebSocketServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
}

void WebSocketServer::run() {
    try {
        server_.listen(port_);
        server_.start_accept();
        Logger::getInstance().log("WebSocket Server started on port " + std::to_string(port_));
        server_.run();
    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("WebSocket Server error: ") + e.what());
    }
}

void WebSocketServer::stop() {
    server_.stop();
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mtx_);
    connections_.insert(hdl);
    Logger::getInstance().log("Client connected.");
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(connections_mtx_);
        auto it = connections_.find(hdl);
        if (it != connections_.end()) {
            connections_.erase(it);
        }
    }
    Logger::getInstance().log("Client disconnected.");

    // Remove client subscriptions
    std::lock_guard<std::mutex> lock_sub(subscriptions_mtx_);
    auto it_sub = client_subscriptions_.find(hdl);
    if (it_sub != client_subscriptions_.end()) {
        std::lock_guard<std::mutex> lock_sym(symbol_mtx_);
        for (const auto& symbol : it_sub->second) {
            symbol_subscription_count_[symbol]--;
            if (symbol_subscription_count_[symbol] == 0) {
                // Unsubscribe from Deribit channel if no clients are subscribed
                // Example: api.unsubscribe(symbol);
                Logger::getInstance().log("No more subscriptions for symbol: " + symbol);
            }
        }
        client_subscriptions_.erase(it_sub);
    }
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg) {
    try {
        auto payload = msg->get_payload();
        Logger::getInstance().log("Received message from client: " + payload);
        // Parse JSON
        auto json_msg = json::parse(payload);

        if (!json_msg.contains("action") || !json_msg.contains("symbols")) {
            // Invalid message format
            json error_response = {
                {"error", "Invalid message format. 'action' and 'symbols' required."}
            };
            server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
            return;
        }

        std::string action = json_msg["action"];
        std::vector<std::string> symbols = json_msg["symbols"].get<std::vector<std::string>>();

        if (action == "subscribe") {
            handle_subscribe(hdl, json_msg);
        } else if (action == "unsubscribe") {
            handle_unsubscribe(hdl, json_msg);
        } else {
            // Unknown action
            json error_response = {
                {"error", "Unknown action. Use 'subscribe' or 'unsubscribe'."}
            };
            server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
        }

    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Error handling message: ") + e.what());
        json error_response = {
            {"error", "Failed to parse message."}
        };
        server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
    }
}

void WebSocketServer::handle_subscribe(websocketpp::connection_hdl hdl, const json& payload) {
    if (!payload.contains("symbols") || !payload["symbols"].is_array()) {
        json error_response = {
            {"error", "'symbols' must be an array."}
        };
        server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
        return;
    }

    std::vector<std::string> symbols = payload["symbols"].get<std::vector<std::string>>();
    {
        std::lock_guard<std::mutex> lock(subscriptions_mtx_);
        for (const auto& symbol : symbols) {
            client_subscriptions_[hdl].insert(symbol);
            {
                std::lock_guard<std::mutex> lock_sym(symbol_mtx_);
                symbol_subscription_count_[symbol]++;
                if (symbol_subscription_count_[symbol] == 1) {
                    // Subscribe to Deribit channel if this is the first subscription
                    // Example: api.subscribe(symbol);
                    Logger::getInstance().log("Subscribed to Deribit channel for symbol: " + symbol);
                }
            }
        }
    }

    // Acknowledge subscription
    json success_response = {
        {"result", symbols},
        {"action", "subscribe"}
    };
    server_.send(hdl, success_response.dump(), websocketpp::frame::opcode::text);
}

void WebSocketServer::handle_unsubscribe(websocketpp::connection_hdl hdl, const json& payload) {
    if (!payload.contains("symbols") || !payload["symbols"].is_array()) {
        json error_response = {
            {"error", "'symbols' must be an array."}
        };
        server_.send(hdl, error_response.dump(), websocketpp::frame::opcode::text);
        return;
    }

    std::vector<std::string> symbols = payload["symbols"].get<std::vector<std::string>>();
    {
        std::lock_guard<std::mutex> lock(subscriptions_mtx_);
        for (const auto& symbol : symbols) {
            auto it = client_subscriptions_.find(hdl);
            if (it != client_subscriptions_.end()) {
                it->second.erase(symbol);
                {
                    std::lock_guard<std::mutex> lock_sym(symbol_mtx_);
                    symbol_subscription_count_[symbol]--;
                    if (symbol_subscription_count_[symbol] == 0) {
                        // Unsubscribe from Deribit channel if no clients are subscribed
                        // Example: api.unsubscribe(symbol);
                        Logger::getInstance().log("Unsubscribed from Deribit channel for symbol: " + symbol);
                        symbol_subscription_count_.erase(symbol);
                    }
                }
            }
        }
    }

    // Acknowledge unsubscription
    json success_response = {
        {"result", symbols},
        {"action", "unsubscribe"}
    };
    server_.send(hdl, success_response.dump(), websocketpp::frame::opcode::text);
}

void WebSocketServer::broadcast(const std::string& symbol, const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mtx_);
    std::lock_guard<std::mutex> lock_sub(subscriptions_mtx_);

    for(auto it : connections_) {
        auto it_sub = client_subscriptions_.find(it);
        if (it_sub != client_subscriptions_.end()) {
            if (it_sub->second.find(symbol) != it_sub->second.end()) {
                server_.send(it, message, websocketpp::frame::opcode::text);
            }
        }
    }
}
