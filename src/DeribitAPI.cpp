// DeribitAPI.cpp

#include "DeribitAPI.hpp"
#include "Logger.hpp"
#include <curl/curl.h>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <iostream>
#include <future>
#include <boost/asio/ssl/context.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

// Helper function to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Constructor
DeribitAPI::DeribitAPI(const std::string& api_key, const std::string& api_secret,
                       const std::string& rest_url, const std::string& websocket_url)
    : api_key_(api_key), api_secret_(api_secret), rest_url_(rest_url), websocket_url_(websocket_url),
      access_token_(""), ws_connected_(false), ws_client_(), ws_thread_() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize WebSocket client
    ws_client_.init_asio();

    // Optional: Configure logging settings
    ws_client_.clear_access_channels(websocketpp::log::alevel::all);
    ws_client_.clear_error_channels(websocketpp::log::elevel::all);

    // Set Global TLS initialization handler
    ws_client_.set_tls_init_handler(std::bind(&DeribitAPI::on_tls_init, this, std::placeholders::_1));

    // Start WebSocket connection thread
    ws_thread_ = std::thread(&DeribitAPI::init_deribit_connection, this);
}

// Destructor
DeribitAPI::~DeribitAPI() {
    // Close WebSocket connection gracefully
    {
        std::lock_guard<std::mutex> lock(ws_mtx_);
        if (ws_connected_) {
            websocketpp::lib::error_code ec;
            ws_client_.close(ws_hdl_, websocketpp::close::status::normal, "Shutting down", ec);
            if (ec) {
                Logger::getInstance().log("Error closing WebSocket: " + ec.message());
            }
            ws_connected_ = false;
        }
    }

    // Stop ASIO loop
    ws_client_.stop();

    // Join WebSocket thread
    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }

    curl_global_cleanup();
}

// Initialize Deribit Connection
void DeribitAPI::init_deribit_connection() {
    while (true) { // Loop to handle reconnection attempts
        websocketpp::lib::error_code ec;

        Logger::getInstance().log("Attempting WebSocket connection to: " + websocket_url_);

        // Create a new connection
        WsClient::connection_ptr con = ws_client_.get_connection(websocket_url_, ec);
        if (ec) {
            Logger::getInstance().log("WebSocket connection creation failed: " + ec.message());
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue; // Retry after delay
        }

        // Since the global TLS handler is already set, no need to set it again per connection

        // Set Open Handler
        con->set_open_handler(std::bind(&DeribitAPI::on_ws_open, this, std::placeholders::_1));

        // Set Fail Handler
        con->set_fail_handler(std::bind(&DeribitAPI::on_ws_fail, this, std::placeholders::_1));

        // Set Close Handler
        con->set_close_handler(std::bind(&DeribitAPI::on_ws_close, this, std::placeholders::_1));

        // Set Message Handler
        con->set_message_handler(std::bind(&DeribitAPI::on_ws_message, this, std::placeholders::_1, std::placeholders::_2));

        // Initiate the connection
        ws_client_.connect(con);
        Logger::getInstance().log("WebSocket connection initiated.");

        // Run the ASIO io_service loop (blocking call)
        try {
            ws_client_.run();
        } catch (const std::exception& e) {
            Logger::getInstance().log(std::string("WebSocket client exception: ") + e.what());
            // Wait before retrying
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        // If run() exits, loop will attempt to reconnect
    }
}

// TLS initialization handler
std::shared_ptr<boost::asio::ssl::context> DeribitAPI::on_tls_init(connection_hdl /*hdl*/) {
    auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);

        // For production, verify the peer
        ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        ctx->set_default_verify_paths();

        Logger::getInstance().log("TLS context initialized successfully.");
    } catch (const std::exception& e) {
        Logger::getInstance().log("TLS Initialization failed: " + std::string(e.what()));
    }

    return ctx;
}

// WebSocket event handlers
void DeribitAPI::on_ws_open(connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(ws_mtx_);
        ws_hdl_ = hdl;
        ws_connected_ = true;
    }
    Logger::getInstance().log("WebSocket connection established.");

    // Resubscribe to previously subscribed channels
    std::lock_guard<std::mutex> lock(subscription_mtx_);
    for (const auto& channel : subscribed_channels_) {
        if (!subscribe(channel)) {
            Logger::getInstance().log("Resubscription failed for channel: " + channel);
        } else {
            Logger::getInstance().log("Resubscribed to channel: " + channel);
        }
    }
}

void DeribitAPI::on_ws_close(connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(ws_mtx_);
        ws_connected_ = false;
    }
    Logger::getInstance().log("WebSocket connection closed.");
    // After ws_client_.run() exits, the loop will attempt to reconnect
}

void DeribitAPI::on_ws_fail(connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(ws_mtx_);
        ws_connected_ = false;
    }
    Logger::getInstance().log("Failed to connect to Deribit WebSocket.");
    // After ws_client_.run() exits, the loop will attempt to reconnect
}

void DeribitAPI::on_ws_message(connection_hdl hdl, message_ptr msg) {
    std::string payload = msg->get_payload();
    Logger::getInstance().log("Received WebSocket message: " + payload);

    try {
        auto json_msg = nlohmann::json::parse(payload);

        if (json_msg.contains("method") && json_msg["method"] == "subscription") {
            // Process real-time market data
            std::string channel = json_msg["params"]["channel"].get<std::string>();
            nlohmann::json data = json_msg["params"]["data"];

            // Invoke the user-defined callback
            if (message_callback_) {
                message_callback_(channel, data.dump());
            }
        } else if (json_msg.contains("result")) {
            // Handle successful subscription or other results
            Logger::getInstance().log("Subscription successful or received result: " + json_msg.dump());
        } else if (json_msg.contains("error")) {
            // Handle errors
            Logger::getInstance().log("WebSocket error: " + json_msg.dump());
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log("WebSocket message parse error: " + std::string(e.what()));
    }
}

// Set the callback for incoming market data
void DeribitAPI::set_message_callback(MessageCallback callback) {
    message_callback_ = callback;
}

// Subscribe to a Deribit channel
bool DeribitAPI::subscribe(const std::string& channel) {
    std::lock_guard<std::mutex> lock(subscription_mtx_);
    if (subscribed_channels_.find(channel) != subscribed_channels_.end()) {
        // Already subscribed
        return true;
    }

    if (!ws_connected_) {
        Logger::getInstance().log("WebSocket not connected. Cannot subscribe to channel: " + channel);
        return false;
    }

    // Create JSON-RPC subscribe request
    nlohmann::json subscribe_request = {
        {"jsonrpc", "2.0"},
        {"id", 1}, // Consider implementing dynamic IDs for multiple requests
        {"method", "public/subscribe"},
        {"params", {
            {"channels", {channel}}
        }}
    };

    std::string message = subscribe_request.dump();

    // Send the subscribe message
    websocketpp::lib::error_code ec;
    ws_client_.send(ws_hdl_, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        Logger::getInstance().log("Failed to send subscribe message: " + ec.message());
        return false;
    }

    subscribed_channels_.insert(channel);
    Logger::getInstance().log("Subscribed to Deribit channel: " + channel);
    return true;
}

// Unsubscribe from a Deribit channel
bool DeribitAPI::unsubscribe(const std::string& channel) {
    std::lock_guard<std::mutex> lock(subscription_mtx_);
    if (subscribed_channels_.find(channel) == subscribed_channels_.end()) {
        // Not subscribed
        return true;
    }

    if (!ws_connected_) {
        Logger::getInstance().log("WebSocket not connected. Cannot unsubscribe from channel: " + channel);
        return false;
    }

    // Create JSON-RPC unsubscribe request
    nlohmann::json unsubscribe_request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/unsubscribe"},
        {"params", {
            {"channels", {channel}}
        }}
    };

    std::string message = unsubscribe_request.dump();

    // Send the unsubscribe message
    websocketpp::lib::error_code ec;
    ws_client_.send(ws_hdl_, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        Logger::getInstance().log("Failed to send unsubscribe message: " + ec.message());
        return false;
    }

    subscribed_channels_.erase(channel);
    Logger::getInstance().log("Unsubscribed from Deribit channel: " + channel);
    return true;
}

// Unsubscribe from all Deribit channels
bool DeribitAPI::unsubscribe_all() {
    std::lock_guard<std::mutex> lock(subscription_mtx_);
    if (subscribed_channels_.empty()) {
        return true;
    }

    if (!ws_connected_) {
        Logger::getInstance().log("WebSocket not connected. Cannot unsubscribe from channels.");
        return false;
    }

    // Create JSON-RPC unsubscribe_all request
    nlohmann::json unsubscribe_all_request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/unsubscribe_all"},
        {"params", {}}
    };

    std::string message = unsubscribe_all_request.dump();

    // Send the unsubscribe_all message
    websocketpp::lib::error_code ec;
    ws_client_.send(ws_hdl_, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        Logger::getInstance().log("Failed to send unsubscribe_all message: " + ec.message());
        return false;
    }

    subscribed_channels_.clear();
    Logger::getInstance().log("Unsubscribed from all Deribit channels.");
    return true;
}

// Place Order
nlohmann::json DeribitAPI::place_order(const std::string& instrument, const std::string& side, double quantity, double price) {
    nlohmann::json params = {
        {"instrument_name", instrument},
        {"direction", side},
        {"amount", quantity},
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
        {"amount", new_quantity},
        {"price", new_price}
    };
    return send_request("private/edit", params, true);
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

// Implement the get_market_data method
nlohmann::json DeribitAPI::get_market_data(const std::string& symbol) {
    nlohmann::json params = {
        {"instrument_name", symbol}
    };
    nlohmann::json response = send_request("public/ticker", params, false);

    if (response.contains("result")) {
        return response["result"];
    } else {
        Logger::getInstance().log("Failed to fetch market data: " + response.dump());
        return nlohmann::json();
    }
}

// Check if the current token is valid
bool DeribitAPI::is_token_valid() {
    std::lock_guard<std::mutex> lock(token_mtx_);
    auto now = std::chrono::system_clock::now();
    // Consider token valid if current time is before expiry minus a buffer (e.g., 60 seconds)
    return !access_token_.empty() && (now + std::chrono::seconds(60) < token_expiry_);
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
