#pragma once


#include "client_interface.hpp"
#include "packet.hpp"
#include "sensor_emulator.hpp"
#include "serializer.hpp"
#include "logger.hpp"
#include "chrono"
#include <boost/asio.hpp>
#include <atomic>
#include <boost/asio/streambuf.hpp>
#include <mutex>
#include <thread>
#include <string>
#include <cstdint>
#include <functional>
#include <chrono>

class ClientA : IClient {
    mutable std::atomic<bool> running_{true};
    mutable std::atomic<bool> disconnecting_{false};
    mutable std::atomic<bool> reconnecting_{false};
    nlohmann::json packet_;
    std::mutex mutex_;                         
    boost::asio::ip::tcp::socket socket_;     
    boost::asio::streambuf buffer_;  
    Logger logger_;                          
    std::string address_;
    std::thread generator_;
    std::chrono::milliseconds ms_;
    uint16_t port_;            
    boost::asio::io_context& io_;  
    boost::asio::ip::tcp::endpoint ep_; 
    
    static constexpr int MAX_RETRIES = 10;
    static constexpr int RETRY_DELAY_SECONDS = 3;

    void start_generating(const std::chrono::milliseconds& ms, 
                        std::function<void()> send_callback);

    void connect() override;
    void reconnect() override;
    void read() override;
    void send() override;

    void handleError(ErrorType type, const std::string& e) override;
    void handleDisconnect() override;

public:
    ClientA(boost::asio::io_context& io, 
            uint16_t port, 
            const std::string& address = "127.0.0.1",
            const std::chrono::milliseconds ms = std::chrono::milliseconds(20));

    ~ClientA();

    ClientA(const ClientA&) = delete;
    ClientA& operator=(const ClientA&) = delete;
    ClientA(ClientA&&) = delete;
    ClientA& operator=(ClientA&&) = delete;

    void stop() override;
    void join() override;

    void start() override;
};