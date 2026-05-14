#pragma once

#include "packet.hpp"
#include "socket_type.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <cstddef>
#include <functional>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <optional>
#include <memory>

class Session : public std::enable_shared_from_this<Session>{    
public:
    using stopCallback = std::function<void(std::shared_ptr<Session>, SocketType, std::optional<boost::asio::ip::tcp::socket>)>;

private:
    boost::asio::ip::tcp::socket socket_A_;
    boost::asio::ip::tcp::socket socket_B_;
    boost::asio::streambuf buffer_A_;
    boost::asio::streambuf buffer_B_;
    std::optional<AccelPacket> last_packet_ = std::nullopt;
    std::atomic<bool> running_ = true;
    stopCallback stopcallback_ = nullptr;

    void readFromA();
    void readFromB();

    bool checkDuplicate(const AccelPacket& cur_packet) const;

    void sendToA(const std::string& json_string);
    void sendToB(const std::string& json_string);

    void stop(const SocketType socket_type);
public:
    Session(boost::asio::ip::tcp::socket socket_A, boost::asio::ip::tcp::socket socket_B,
            stopCallback callback);

    void start();

    void stop();
};