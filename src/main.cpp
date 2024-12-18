// main.cpp

#include "Config.hpp"
#include "DeribitAPI.hpp"
#include "OrderManager.hpp"
#include "WebSocketServer.hpp"
#include "Logger.hpp"
#include <thread>
#include <iostream>
#include <chrono>

int main() {
    try {
        // Load configuration
        Config config = Config::load("config.json");

        // Initialize Logger
        Logger::getInstance().log("Starting DeribitTrader...");

        // Initialize Deribit API with four arguments
        DeribitAPI api(config.api_key, config.api_secret, config.rest_url, config.websocket_url);

        // Authenticate
        if (!api.authenticate()) {
            Logger::getInstance().log("Authentication failed. Exiting application.");
            return 1;
        }

        // Initialize Order Manager
        OrderManager order_manager(api);

        // Initialize WebSocket Server
        WebSocketServer ws_server(config.websocket_port);
        std::thread ws_thread([&ws_server]() {
            ws_server.run();
        });

        // Set up the callback to broadcast incoming market data to WebSocket clients
        api.set_message_callback([&ws_server](const std::string& channel, const std::string& message) {
            // Extract the symbol from the channel name
            // Example channel format: "book.BTC-PERPETUAL.raw"
            std::string symbol;
            size_t first_dot = channel.find('.');
            size_t second_dot = channel.find('.', first_dot + 1);
            if (first_dot != std::string::npos && second_dot != std::string::npos) {
                symbol = channel.substr(first_dot + 1, second_dot - first_dot - 1);
            } else {
                symbol = "unknown";
            }

            // Broadcast the message to subscribed WebSocket clients
            ws_server.broadcast(symbol, message);
        });

        // Example usage
        std::string instrument = "ETH-PERPETUAL";
        std::string side = "sell";
        double quantity = 40.0;
        double price = 4.0;

        nlohmann::json order_response = api.place_order(instrument, side, quantity, price);
        if (order_response.contains("result")) {
            std::string order_id = order_response["result"]["order"]["order_id"].get<std::string>();
            std::cout << "Order placed with ID: " << order_id << std::endl;
        } else {
            std::cout << "Failed to place order: " << order_response.dump() << std::endl;
        }

        // Fetch and print order book
        nlohmann::json orderbook = api.get_orderbook(instrument);
        std::cout << "Order Book: " << orderbook.dump(4) << std::endl;

        // Subscribe to the order book channel for the instrument
        std::string subscribe_channel = "book." + instrument + ".100ms";
        if (api.subscribe(subscribe_channel)) {
            Logger::getInstance().log("Subscribed to channel: " + subscribe_channel);
        } else {
            Logger::getInstance().log("Failed to subscribe to channel: " + subscribe_channel);
        }

        // Keep the main thread alive
        ws_thread.join();

    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Exception: ") + e.what());
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
