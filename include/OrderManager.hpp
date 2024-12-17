#ifndef ORDERMANAGER_HPP
#define ORDERMANAGER_HPP

#include "DeribitAPI.hpp"
#include <string>
#include <unordered_map>
#include <mutex>

struct Order {
    std::string order_id;
    std::string instrument;
    std::string side;
    double quantity;
    double price;
};

class OrderManager {
public:
    OrderManager(DeribitAPI& api);
    
    std::string place_order(const std::string& instrument, const std::string& side, double quantity, double price);
    bool cancel_order(const std::string& order_id);
    bool modify_order(const std::string& order_id, double new_quantity, double new_price);
    std::unordered_map<std::string, Order> get_current_orders();
    
private:
    DeribitAPI& api_;
    std::unordered_map<std::string, Order> orders_;
    std::mutex mtx_;
};

#endif // ORDERMANAGER_HPP
