#include "WebSocketServer.hpp"
#include "Logger.hpp"
#include <iostream>

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
    std::lock_guard<std::mutex> lock(connections_mtx_);
    connections_.erase(hdl);
    Logger::getInstance().log("Client disconnected.");
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg) {
    // Handle incoming messages from clients (e.g., subscriptions)
    Logger::getInstance().log("Received message from client: " + msg->get_payload());
    // Implement subscription logic here
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mtx_);
    for(auto it : connections_) {
        server_.send(it, message, websocketpp::frame::opcode::text);
    }
}

void WebSocketServer::subscribe(const std::string& symbol) {
    // Implement subscription logic
}

void WebSocketServer::unsubscribe(const std::string& symbol) {
    // Implement unsubscription logic
}
