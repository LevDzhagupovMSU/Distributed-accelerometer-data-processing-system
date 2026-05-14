#pragma once

#include "session.hpp"
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <memory>
#include <syslog.h>
#include <algorithm>
#include <cstdint>
#include <queue>
#include <vector>

class Server{
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::acceptor acceptor_A_;
    boost::asio::ip::tcp::acceptor acceptor_B_;

    std::queue<boost::asio::ip::tcp::socket> waiting_A_;
    std::queue<boost::asio::ip::tcp::socket> waiting_B_;

    std::vector<std::shared_ptr<Session>> sessions_;
    std::string address_;

    void accept_A();
    void accept_B();

    void create_session();
    void remove_session(std::shared_ptr<Session> session, SocketType socket_type, 
                        std::optional<boost::asio::ip::tcp::socket> socket);
public:
    Server(boost::asio::io_context& io,
           uint16_t port_A, uint16_t port_B,
           const std::string& address = "127.0.0.1");
    
    ~Server();
    
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void start();
    void stop();
};