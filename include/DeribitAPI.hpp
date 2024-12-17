#ifndef DERIBITAPI_HPP
#define DERIBITAPI_HPP

#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <mutex>
#include <chrono>

class DeribitAPI {
public:
    using Callback = std::function<void(const nlohmann::json&)>;

    DeribitAPI(const std::string& api_key, const std::string& api_secret, const std::string& rest_url);
    ~DeribitAPI();

    // Authentication
    bool authenticate();

    // Order Management
    nlohmann::json place_order(const std::string& instrument, const std::string& side, double quantity, double price);
    nlohmann::json cancel_order(const std::string& order_id);
    nlohmann::json modify_order(const std::string& order_id, double new_quantity, double new_price);
    nlohmann::json get_orderbook(const std::string& instrument);
    nlohmann::json get_positions();

private:
    std::string api_key_;
    std::string api_secret_;
    std::string rest_url_;

    std::string access_token_;
    std::chrono::system_clock::time_point token_expiry_;

    std::mutex token_mtx_;

    // Helper functions
    nlohmann::json send_request(const std::string& method, const nlohmann::json& params, bool requires_auth = false);
    bool is_token_valid();
};

#endif // DERIBITAPI_HPP
