#include "session.hpp"
#include "packet.hpp"
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/system/detail/error_code.hpp>
#include <memory>
#include <optional>
#include <utility>
#include <syslog.h>

Session::Session(boost::asio::ip::tcp::socket socket_A, boost::asio::ip::tcp::socket socket_B,
                 stopCallback callback) : socket_A_{std::move(socket_A)}, 
                                          socket_B_{std::move(socket_B)},
                                          stopcallback_{std::move(callback)} {
    syslog(LOG_DEBUG, "[SESSION] Session created");
};

void Session::start(){
    syslog(LOG_DEBUG, "[SESSION] Starting session");
    readFromA();
    readFromB();
}

void Session::readFromA(){
    auto self = shared_from_this();

    boost::asio::async_read_until(socket_A_, buffer_A_,'\n',
    [self](const boost::system::error_code& ec, size_t){
        if(!self->running_) return;

        if(!ec){
            try {
                std::istream is(&self->buffer_A_);

                std::string line;
                std::getline(is, line);

                auto json = nlohmann::json::parse(line);

                if (json.contains("version")) {
                    uint32_t version = json["version"];
                    if (version != 1) { 
                        syslog(LOG_WARNING, "[SESSION] Unsupported protocol version: %u from client A", version);
                        self->stop();
                    }
                }

                auto json_string = json.dump();
                json_string.push_back('\n');
                AccelPacket packet{json["x"],json["y"],json["z"], json["version"]};
                packet.timestamp = json["timestamp"];

                if (!self->checkDuplicate(packet)) {
                    self->last_packet_ = packet;
                    self->sendToB(json_string);
                }else{
                    syslog(LOG_DEBUG, "[SESSION] Duplicate packet detected, skipped: x=%.4f, y=%.4f, z=%.4f",packet.x, packet.y, packet.z);
                }
            }catch(const std::exception& e){
                syslog(LOG_ERR, "[SESSION] readFromA parse error: %s", e.what());
                self->stop(SocketType::SOCKET_A);
            }

            self->readFromA();
        }else{
            syslog(LOG_WARNING, "[SESSION] readFromA error: %s", ec.message().c_str());
            self->stop(SocketType::SOCKET_A);
        }
    });
}

void Session::readFromB(){
    auto self = shared_from_this();

    boost::asio::async_read_until(socket_B_, buffer_B_, '\n',
    [self](const boost::system::error_code& ec, size_t){
        if(!self->running_) return;
        
        if(!ec){
            try{
                std::istream is(&self->buffer_B_);

                std::string line;
                std::getline(is, line);

                auto json_data = nlohmann::json::parse(line);

                if (json_data.contains("version")) {
                    int version = json_data["version"];
                    if (version != 1) {
                        syslog(LOG_WARNING, "[SESSION] Unsupported protocol version: %u from client B", version);
                        self->stop(SocketType::SOCKET_B);
                    }
                }

                auto json_string = json_data.dump();
                json_string.push_back('\n');

                self->sendToA(json_string);
                self->readFromB();
            }catch(const std::exception& e){
                syslog(LOG_ERR, "[SESSION] readFromB parse error: %s", e.what());
                self->stop(SocketType::SOCKET_B);
            }
        }else{
            syslog(LOG_WARNING, "[SESSION] readFromB error: %s", ec.message().c_str());
            self->stop(SocketType::SOCKET_B);
        }
    });
}

void Session::sendToA(const std::string& json_string){
    auto self = shared_from_this();

    auto ptr_json_string = std::make_shared<std::string>(json_string);
    boost::asio::async_write(socket_A_, boost::asio::buffer(ptr_json_string->data(), ptr_json_string->size()), 
    [self, ptr_json_string](const boost::system::error_code& ec, size_t){
        if(ec && self->running_){
            syslog(LOG_WARNING, "[SESSION] sendToA error: %s", ec.message().c_str());
            self->stop();
        }
    });
}

void Session::sendToB(const std::string& json_string){
    auto self = shared_from_this();

    auto ptr_json_string = std::make_shared<std::string>(json_string);
    boost::asio::async_write(socket_B_, boost::asio::buffer(ptr_json_string->data(), ptr_json_string->size()), 
    [self, ptr_json_string](const boost::system::error_code& ec, size_t){
        if(ec && self->running_){
            syslog(LOG_WARNING, "[SESSION] sendToB error: %s", ec.message().c_str());
            self->stop();
        }
    });
}

bool Session::checkDuplicate(const AccelPacket& cur_packet) const{
    if(!last_packet_.has_value()) 
        return false;

    auto [t,x, y, z, ver] = last_packet_.value();

    double epsilon = 1e-4;

    return std::abs(x - cur_packet.x) <= epsilon && std::abs(y - cur_packet.y) <= epsilon && std::abs(z - cur_packet.z) <= epsilon;
}

void Session::stop(){
    if(running_){
        running_ = false;

        syslog(LOG_INFO, "[SESSION] Stopping session...");

        boost::system::error_code ec1, ec2;
        socket_A_.close(ec1);
        socket_B_.close(ec2);

        if (ec1){
            syslog(LOG_WARNING, "[SESSION] socket_A close error: %s", ec1.message().c_str());
        }
        if (ec2){
            syslog(LOG_WARNING, "[SESSION] socket_B close error: %s", ec2.message().c_str());
        }

        if (stopcallback_){
            syslog(LOG_DEBUG, "[SESSION] Calling stop callback");
            stopcallback_(shared_from_this(), SocketType::ALL, std::nullopt);
        }
    }
    syslog(LOG_INFO, "[SESSION] Session stopped");
}

void Session::stop(const SocketType socket_type){
    if(!running_) return;

    running_ = false;

    boost::system::error_code ec;

    switch (socket_type){
        case SocketType::SOCKET_A:
            syslog(LOG_INFO, "[SESSION] Stopping session from Client_A...");
            socket_A_.close(ec);

            if (ec){
                syslog(LOG_WARNING, "[SESSION] socket_A close error: %s", ec.message().c_str());
            }

            if (stopcallback_){
                syslog(LOG_DEBUG, "[SESSION] Calling stop callback from socket_A");
                stopcallback_(shared_from_this(), SocketType::SOCKET_B, std::move(socket_B_));
            }
        break;

        case SocketType::SOCKET_B:
            syslog(LOG_INFO, "[SESSION] Stopping session from Client_B...");
            socket_B_.close(ec);

            if (ec){
                syslog(LOG_WARNING, "[SESSION] socket_B close error: %s", ec.message().c_str());
            }

            if (stopcallback_){
                syslog(LOG_DEBUG, "[SESSION] Calling stop callback from socket_B");
                stopcallback_(shared_from_this(), SocketType::SOCKET_A, std::move(socket_A_));
            }
        break;

        case SocketType::ALL:
            stop();
        break;
    }
}