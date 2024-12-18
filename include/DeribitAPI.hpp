// DeribitAPI.hpp

#ifndef DERIBITAPI_HPP
#define DERIBITAPI_HPP

#include <string>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <functional>
#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <boost/asio/ssl/context.hpp>

// Forward declaration for WebSocket++ types
typedef websocketpp::connection_hdl connection_hdl;

class DeribitAPI {
public:
    // Type definitions
    typedef std::function<void(const std::string&, const std::string&)> MessageCallback;
    typedef websocketpp::client<websocketpp::config::asio_tls_client> WsClient;
    typedef websocketpp::client<websocketpp::config::asio_tls_client>::message_ptr message_ptr;

    // Constructor and Destructor
    DeribitAPI(const std::string& api_key, const std::string& api_secret,
               const std::string& rest_url, const std::string& websocket_url);
    ~DeribitAPI();

    // Public methods
    void set_message_callback(MessageCallback callback);
    bool subscribe(const std::string& channel);
    bool unsubscribe(const std::string& channel);
    bool unsubscribe_all();

    // Order Management Methods
    nlohmann::json place_order(const std::string& instrument, const std::string& side, double quantity, double price);
    nlohmann::json cancel_order(const std::string& order_id);
    nlohmann::json modify_order(const std::string& order_id, double new_quantity, double new_price);

    // Data Retrieval Methods
    nlohmann::json get_orderbook(const std::string& instrument);
    nlohmann::json get_positions();
    nlohmann::json get_market_data(const std::string& symbol);

    // Authentication Method
    bool authenticate();

private:
    // Private methods
    void init_deribit_connection();
    std::shared_ptr<boost::asio::ssl::context> on_tls_init(connection_hdl hdl);
    void on_ws_open(connection_hdl hdl);
    void on_ws_close(connection_hdl hdl);
    void on_ws_fail(connection_hdl hdl);
    void on_ws_message(connection_hdl hdl, message_ptr msg);
    bool is_token_valid();
    nlohmann::json send_request(const std::string& method, const nlohmann::json& params, bool requires_auth);

    // Member variables
    std::string api_key_;
    std::string api_secret_;
    std::string rest_url_;
    std::string websocket_url_;
    std::string access_token_;
    std::chrono::system_clock::time_point token_expiry_;
    bool ws_connected_;
    connection_hdl ws_hdl_;
    WsClient ws_client_;
    std::thread ws_thread_;
    std::mutex ws_mtx_;
    std::mutex subscription_mtx_;
    std::mutex token_mtx_;
    std::unordered_set<std::string> subscribed_channels_;
    MessageCallback message_callback_;
};

#endif // DERIBITAPI_HPP
