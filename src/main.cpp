#include <iostream>
#include <string>
#include "Config.hpp"
#include "DeribitAPI.hpp"
#include "OrderManager.hpp"
#include "WebSocketServer.hpp"
#include "Logger.hpp"
#include <thread>
#include <algorithm>

int main() {
    try {
        // Load configuration
        Config config = Config::load("config.json");

        // Initialize Logger
        Logger::getInstance().log("Starting DeribitTrader...");

        // Initialize Deribit API
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

        // CLI Loop
        std::string command;
        while (true) {
            std::cout << "\nPress the respective numbers to activate the commands:\n"
                      << "place_order: 1\n"
                      << "cancel_order: 2\n"
                      << "modify_order: 3\n"
                      << "get_orderbook: 4\n"
                      << "view_positions: 5\n"
                      << "subscribe: 6\n"
                      << "unsubscribe: 7\n"
                      << "exit: 8\n"
                      << "Enter Command: ";
            std::getline(std::cin, command);

            // Convert command to lowercase for consistency
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);

            if (command == "1") {
                std::string instrument, side;
                double quantity, price;

                std::cout << "Enter Instrument (e.g., ETH-PERPETUAL): ";
                std::getline(std::cin, instrument);
                std::cout << "Enter Side (buy/sell): ";
                std::getline(std::cin, side);
                std::cout << "Enter Quantity: ";
                std::cin >> quantity;
                std::cout << "Enter Price: ";
                std::cin >> price;
                std::cin.ignore(); // Clear newline character

                std::string order_id = order_manager.place_order(instrument, side, quantity, price);
                if (!order_id.empty()) {
                    std::cout << "Order placed successfully. Order ID: " << order_id << std::endl;
                } else {
                    std::cout << "Failed to place order. Check logs for details." << std::endl;
                }
            }
            else if (command == "2") {
                std::string order_id;
                std::cout << "Enter Order ID to cancel: ";
                std::getline(std::cin, order_id);
                if (order_manager.cancel_order(order_id)) {
                    std::cout << "Order canceled successfully." << std::endl;
                } else {
                    std::cout << "Failed to cancel order. Check logs for details." << std::endl;
                }
            }
            else if (command == "3") {
                std::string order_id;
                double new_quantity, new_price;
                std::cout << "Enter Order ID to modify: ";
                std::getline(std::cin, order_id);
                std::cout << "Enter New Quantity: ";
                std::cin >> new_quantity;
                std::cout << "Enter New Price: ";
                std::cin >> new_price;
                std::cin.ignore(); // Clear newline character

                if (order_manager.modify_order(order_id, new_quantity, new_price)) {
                    std::cout << "Order modified successfully." << std::endl;
                } else {
                    std::cout << "Failed to modify order. Check logs for details." << std::endl;
                }
            }
            else if (command == "4") {
                std::string instrument;
                std::cout << "Enter Instrument (e.g., ETH-PERPETUAL): ";
                std::getline(std::cin, instrument);
                nlohmann::json orderbook = api.get_orderbook(instrument);
                if (orderbook.contains("result")) {
                    std::cout << "Order Book for " << instrument << ":\n" << orderbook.dump(4) << std::endl;
                } else {
                    std::cout << "Failed to fetch order book. Check logs for details." << std::endl;
                }
            }
            else if (command == "5") {
                nlohmann::json positions = api.get_positions();
                if (positions.contains("result")) {
                    std::cout << "Current Positions:\n" << positions.dump(4) << std::endl;
                } else {
                    std::cout << "Failed to fetch positions. Check logs for details." << std::endl;
                }
            }
            else if (command == "6") {
                std::string symbols_input;
                std::cout << "Enter Symbols to subscribe (comma-separated, e.g., BTC-PERPETUAL,ETH-PERPETUAL): ";
                std::getline(std::cin, symbols_input);
                // Split symbols by comma
                std::vector<std::string> symbols;
                size_t pos = 0;
                while ((pos = symbols_input.find(',')) != std::string::npos) {
                    symbols.push_back(symbols_input.substr(0, pos));
                    symbols_input.erase(0, pos + 1);
                }
                symbols.push_back(symbols_input); // Add the last symbol

                for(auto &symbol : symbols) {
                    // Trim whitespace
                    symbol.erase(symbol.find_last_not_of(" \n\r\t")+1);
                    symbol.erase(0, symbol.find_first_not_of(" \n\r\t"));
                    if(api.subscribe(symbol + ".100ms")) { // Example channel format
                        std::cout << "Subscribed to symbol: " << symbol << std::endl;
                    }
                    else {
                        std::cout << "Failed to subscribe to symbol: " << symbol << std::endl;
                    }
                }
            }
            else if (command == "7") {
                std::string symbols_input;
                std::cout << "Enter Symbols to unsubscribe (comma-separated, e.g., BTC-PERPETUAL,ETH-PERPETUAL): ";
                std::getline(std::cin, symbols_input);
                // Split symbols by comma
                std::vector<std::string> symbols;
                size_t pos = 0;
                while ((pos = symbols_input.find(',')) != std::string::npos) {
                    symbols.push_back(symbols_input.substr(0, pos));
                    symbols_input.erase(0, pos + 1);
                }
                symbols.push_back(symbols_input); // Add the last symbol

                for(auto &symbol : symbols) {
                    // Trim whitespace
                    symbol.erase(symbol.find_last_not_of(" \n\r\t")+1);
                    symbol.erase(0, symbol.find_first_not_of(" \n\r\t"));
                    if(api.unsubscribe(symbol + ".100ms")) { // Example channel format
                        std::cout << "Unsubscribed from symbol: " << symbol << std::endl;
                    }
                    else {
                        std::cout << "Failed to unsubscribe from symbol: " << symbol << std::endl;
                    }
                }
            }
            else if (command == "8") {
                std::cout << "Exiting application..." << std::endl;
                return 0;
            }
            else {
                std::cout << "Unknown command. Please try again." << std::endl;
            }
        }

        // Cleanup
        ws_server.stop();
        if (ws_thread.joinable()) {
            ws_thread.join();
        }

    } catch (const std::exception& e) {
        Logger::getInstance().log(std::string("Exception: ") + e.what());
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
