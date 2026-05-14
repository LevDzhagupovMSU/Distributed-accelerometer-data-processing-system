#pragma once

#include "client_interface.hpp"
#include "serializer.hpp"
#include "packet.hpp"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <optional>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <memory>

class ClientB : IClient {
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::endpoint ep_;
    boost::asio::streambuf buffer_;
    std::optional<AccelModule> packet_ = std::nullopt;
    std::mutex mutex_;
    std::string address_;
    std::chrono::milliseconds ms_;
    mutable std::atomic<bool> running_ = true;
    mutable std::atomic<bool> disconnecting_{false};
    mutable std::atomic<bool> reconnecting_{false};
    uint16_t port_;

    static constexpr int MAX_RETRIES = 10;
    static constexpr int RETRY_DELAY_SECONDS = 3;

    double get_acceleration(double x, double y, double z) const; 

    void connect() override;
    void reconnect() override;
    void read() override;
    void send() override;

    void handleError(ErrorType type, const std::string& e) override;
    void handleDisconnect() override;

public:
    ClientB(boost::asio::io_context& io, 
            uint16_t port, 
            std::string address = "127.0.0.1",
            const std::chrono::milliseconds ms = std::chrono::milliseconds(20));

    ~ClientB();

    ClientB(const ClientB&) = delete;
    ClientB& operator=(const ClientB&) = delete;
    ClientB(ClientB&&) = delete;
    ClientB& operator=(ClientB&&) = delete;

    void stop() override;
    void join() override;

    void start() override;
};