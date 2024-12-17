#include "OrderManager.hpp"
#include "Logger.hpp"
#include <iostream>

OrderManager::OrderManager(DeribitAPI& api) : api_(api) {}

std::string OrderManager::place_order(const std::string& instrument, const std::string& side, double quantity, double price) {
    Logger::getInstance().log("Attempting to place order: Instrument=" + instrument + ", Side=" + side + ", Quantity=" + std::to_string(quantity) + ", Price=" + std::to_string(price));

    auto response = api_.place_order(instrument, side, quantity, price);

    // Check if "result" exists and is an object
    if (response.contains("result") && response["result"].is_object()) {
        // Check if "order_id" exists
        if (response["result"].contains("order") && response["result"]["order"].contains("order_id") && response["result"]["order"]["order_id"].is_string()) {
            std::string order_id = response["result"]["order"]["order_id"].get<std::string>();
            std::cout << "Order ID: " << order_id << std::endl;

            std::lock_guard<std::mutex> lock(mtx_);
            orders_[order_id] = Order{order_id, instrument, side, quantity, price};
            Logger::getInstance().log("Placed order successfully. Order ID: " + order_id);
            return order_id;
        } else {
            Logger::getInstance().log("place_order response missing 'order_id'. Response: " + response.dump());
        }
    } else if (response.contains("error")) {
        // Log the error message from the API
        std::string error_message = response["error"].contains("message") ? response["error"]["message"].get<std::string>() : "Unknown error";
        Logger::getInstance().log("Failed to place order. API Error: " + error_message);
    } else {
        Logger::getInstance().log("place_order response missing 'result'. Response: " + response.dump());
    }

    return "";
}

bool OrderManager::cancel_order(const std::string& order_id) {
    Logger::getInstance().log("Attempting to cancel order: Order ID=" + order_id);
    
    auto response = api_.cancel_order(order_id);
    Logger::getInstance().log("cancel_order response: " + response.dump());
    
    if (response.contains("result") && response["result"].is_boolean()) {
        if (response["result"].get<bool>()) {
            std::lock_guard<std::mutex> lock(mtx_);
            orders_.erase(order_id);
            Logger::getInstance().log("Cancelled order successfully. Order ID: " + order_id);
            return true;
        } else {
            Logger::getInstance().log("API reported failure to cancel order. Order ID: " + order_id);
        }
    } else if (response.contains("error")) {
        std::string error_message = response["error"].contains("message") ? response["error"]["message"].get<std::string>() : "Unknown error";
        Logger::getInstance().log("Failed to cancel order. API Error: " + error_message);
    } else {
        Logger::getInstance().log("cancel_order response missing 'result'. Response: " + response.dump());
    }

    return false;
}

bool OrderManager::modify_order(const std::string& order_id, double new_quantity, double new_price) {
    Logger::getInstance().log("Attempting to modify order: Order ID=" + order_id + ", New Quantity=" + std::to_string(new_quantity) + ", New Price=" + std::to_string(new_price));
    
    auto response = api_.modify_order(order_id, new_quantity, new_price);
    
    if (response.contains("result") && response["result"].is_object()) {
        if (response["result"].contains("order") && response["result"]["order"].contains("order_id") && response["result"]["order"]["order_id"].is_string()) {
            std::string modified_order_id = response["result"]["order"]["order_id"].get<std::string>();
            std::lock_guard<std::mutex> lock(mtx_);
            orders_[modified_order_id].quantity = new_quantity;
            orders_[modified_order_id].price = new_price;
            Logger::getInstance().log("Modified order successfully. Order ID: " + modified_order_id);
            return true;
        } else {
            Logger::getInstance().log("modify_order response missing 'order_id'. Response: " + response.dump());
        }
    } else if (response.contains("error")) {
        std::string error_message = response["error"].contains("message") ? response["error"]["message"].get<std::string>() : "Unknown error";
        Logger::getInstance().log("Failed to modify order. API Error: " + error_message);
    } else {
        Logger::getInstance().log("modify_order response missing 'result'. Response: " + response.dump());
    }

    return false;
}

std::unordered_map<std::string, Order> OrderManager::get_current_orders() {
    std::lock_guard<std::mutex> lock(mtx_);
    return orders_;
}
