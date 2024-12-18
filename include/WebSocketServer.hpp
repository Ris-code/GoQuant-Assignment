// WebSocketServer.hpp

#ifndef WEBSOCKETSERVER_HPP
#define WEBSOCKETSERVER_HPP

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <string>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp> // Include JSON library

typedef websocketpp::server<websocketpp::config::asio> Server;

// Custom equality comparator for websocketpp::connection_hdl
struct connection_hdl_equal {
    bool operator()(const websocketpp::connection_hdl& a, const websocketpp::connection_hdl& b) const {
        auto sa = a.lock();
        auto sb = b.lock();
        return sa.get() == sb.get();
    }
};

// Add the hash specialization within the std namespace
namespace std {
    template <>
    struct hash<websocketpp::connection_hdl> {
        std::size_t operator()(const websocketpp::connection_hdl& hdl) const {
            // Lock the weak_ptr to obtain a shared_ptr
            auto sp = hdl.lock();
            // Hash the raw pointer held by the shared_ptr
            return std::hash<void*>()(sp.get());
        }
    };
}

class WebSocketServer {
public:
    WebSocketServer(int port);
    void run();
    void stop();
    
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    
    // Broadcast market data to subscribed clients
    void broadcast(const std::string& symbol, const std::string& message);
    
private:
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg);
    
    void handle_subscribe(websocketpp::connection_hdl hdl, const nlohmann::json& payload);
    void handle_unsubscribe(websocketpp::connection_hdl hdl, const nlohmann::json& payload);
    
    Server server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex connections_mtx_;
    
    // Mapping from connection handle to subscribed symbols
    std::unordered_map<
        websocketpp::connection_hdl, 
        std::unordered_set<std::string>, 
        std::hash<websocketpp::connection_hdl>, 
        connection_hdl_equal
    > client_subscriptions_;
    std::mutex subscriptions_mtx_;
    
    // Mapping from symbol to number of subscriptions
    std::unordered_map<std::string, int> symbol_subscription_count_;
    std::mutex symbol_mtx_;
    
    int port_;
};

#endif // WEBSOCKETSERVER_HPP
