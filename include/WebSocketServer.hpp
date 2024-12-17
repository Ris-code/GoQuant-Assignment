#ifndef WEBSOCKETSERVER_HPP
#define WEBSOCKETSERVER_HPP

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <string>
#include <mutex>
#include <thread>

typedef websocketpp::server<websocketpp::config::asio> Server;

class WebSocketServer {
public:
    WebSocketServer(int port);
    void run();
    void stop();
    
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    
    // Broadcast market data to all clients
    void broadcast(const std::string& message);
    
private:
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg);
    
    Server server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex connections_mtx_;
    int port_;
};

#endif // WEBSOCKETSERVER_HPP
