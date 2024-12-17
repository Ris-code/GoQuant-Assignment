#include "DeribitAPI.hpp"
#include "Logger.hpp"
#include <curl/curl.h>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// Initialize CURL globally
DeribitAPI::DeribitAPI(const std::string& api_key, const std::string& api_secret, const std::string& rest_url)
    : api_key_(api_key), api_secret_(api_secret), rest_url_(rest_url), access_token_(""), token_expiry_(std::chrono::system_clock::now()) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DeribitAPI::~DeribitAPI() {
    curl_global_cleanup();
}

// Helper function to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Check if the current token is valid
bool DeribitAPI::is_token_valid() {
    std::lock_guard<std::mutex> lock(token_mtx_);
    auto now = std::chrono::system_clock::now();
    // Consider token valid if current time is before expiry minus a buffer (e.g., 60 seconds)
    return access_token_.empty() ? false : (now + std::chrono::seconds(60) < token_expiry_);
}

// Authenticate and obtain access token
bool DeribitAPI::authenticate() {
    std::lock_guard<std::mutex> lock(token_mtx_);

    // Prepare authentication request
    nlohmann::json auth_params = {
        {"client_id", api_key_},
        {"client_secret", api_secret_},
        {"grant_type", "client_credentials"}
    };

    nlohmann::json auth_response = send_request("public/auth", auth_params, false);

    if (auth_response.contains("result")) {
        access_token_ = auth_response["result"]["access_token"].get<std::string>();
        int expires_in = auth_response["result"]["expires_in"].get<int>(); // in seconds
        token_expiry_ = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);
        Logger::getInstance().log("Authentication successful. Access token acquired.");
        return true;
    } else {
        Logger::getInstance().log("Authentication failed: " + auth_response.dump());
        return false;
    }
}

// Send API request
nlohmann::json DeribitAPI::send_request(const std::string& method, const nlohmann::json& params, bool requires_auth) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    if(curl) {
        nlohmann::json request_json;
        request_json["jsonrpc"] = "2.0";
        request_json["id"] = 1; // You can implement dynamic IDs if needed
        request_json["method"] = method;
        request_json["params"] = params;

        std::string post_fields = request_json.dump();

        curl_easy_setopt(curl, CURLOPT_URL, rest_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        if (requires_auth) {
            // Ensure token is valid
            if (!is_token_valid()) {
                if (!authenticate()) {
                    Logger::getInstance().log("Failed to authenticate before making API request.");
                    curl_easy_cleanup(curl);
                    curl_slist_free_all(headers);
                    return nlohmann::json();
                }
            }

            headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token_).c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            Logger::getInstance().log("CURL error: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    // Parse the response
    try {
        return nlohmann::json::parse(readBuffer);
    } catch (const std::exception& e) {
        Logger::getInstance().log("JSON parse error: " + std::string(e.what()));
        return nlohmann::json();
    }
}

// Place Order
nlohmann::json DeribitAPI::place_order(const std::string& instrument, const std::string& side, double quantity, double price) {
    nlohmann::json params = {
        {"instrument_name", instrument},
        {"direction", side},
        {"amount", quantity},
        {"contracts", quantity},
        {"price", price}
    };
    return send_request("private/buy", params, true);
}

// Cancel Order
nlohmann::json DeribitAPI::cancel_order(const std::string& order_id) {
    nlohmann::json params = {
        {"order_id", order_id}
    };
    return send_request("private/cancel", params, true);
}

// Modify Order
nlohmann::json DeribitAPI::modify_order(const std::string& order_id, double new_quantity, double new_price) {
    nlohmann::json params = {
        {"order_id", order_id},
        {"quantity", new_quantity},
        {"price", new_price}
    };
    return send_request("private/modify", params, true);
}

// Get Order Book
nlohmann::json DeribitAPI::get_orderbook(const std::string& instrument) {
    nlohmann::json params = {
        {"instrument_name", instrument}
    };
    return send_request("public/get_order_book", params, false);
}

// Get Positions
nlohmann::json DeribitAPI::get_positions() {
    nlohmann::json params = {};
    return send_request("private/get_positions", params, true);
}
