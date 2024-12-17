#include "Config.hpp"
#include "DeribitAPI.hpp"
#include "OrderManager.hpp"
#include "WebSocketServer.hpp"
#include "Logger.hpp"
#include <thread>
#include <iostream>

int main() {
    try {
        // Load configuration
        Config config = Config::load("config.json");
        
        // Initialize Logger
        Logger::getInstance().log("Starting DeribitTrader...");
        
        // Initialize Deribit API
        DeribitAPI api(config.api_key, config.api_secret, config.rest_url);
        
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
        
        // Example usage
        std::string instrument = "ETH-PERPETUAL";
        std::string side = "sell";
        double quantity = 40.0;
        double price = 4.0;
        
        std::string order_id = order_manager.place_order(instrument, side, quantity, price);
        if (!order_id.empty()) {
            std::cout << "Order placed with ID: " << order_id << std::endl;
        }
        
        // Fetch and print order book
        auto orderbook = api.get_orderbook(instrument);
        std::cout << "Order Book: " << orderbook.dump(4) << std::endl;
        
        // Keep the main thread alive
        ws_thread.join();
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Exception: ") + e.what());
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
