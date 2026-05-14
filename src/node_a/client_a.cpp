#include "client_a.hpp"
#include "packet.hpp"
#include <chrono>
#include <istream>
#include <string>
#include <syslog.h>


ClientA::ClientA(boost::asio::io_context& io, 
            uint16_t port, 
            const std::string& address, std::chrono::milliseconds ms) : io_{io}, port_{port}, 
                                          address_{address},
                                          socket_{io},
                                          logger_{"/var/log/task2/accel/module.log"},
                                          ms_{ms}{
    ep_ = std::move(boost::asio::ip::tcp::endpoint{boost::asio::ip::make_address_v4(address), port});
    syslog(LOG_INFO, "[CLIENT A] Created, port=%d, address=%s", port, address.c_str());
};

ClientA::~ClientA() {
    syslog(LOG_INFO, "[CLIENT A] ~ClientA() called");
    stop();
    join();
}

void ClientA::connect(){
    try{
        socket_.connect(ep_);
        syslog(LOG_INFO, "[CLIENT A] Connected to server at %s:%d", address_.c_str(), port_);
    }catch(const boost::system::system_error& er){
        syslog(LOG_ERR, "[CLIENT A] Connection failed: %s", er.what());
        throw er;
    }
}

void ClientA::reconnect() {
    if (reconnecting_.exchange(true)) return;
    
    syslog(LOG_INFO, "[CLIENT A] Reconnecting... (max attempts: %d)", MAX_RETRIES);
    
    std::thread([this]() {
        for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) {
            syslog(LOG_INFO, "[CLIENT A] Reconnect attempt %d/%d", attempt, MAX_RETRIES);
            
            std::this_thread::sleep_for(std::chrono::seconds(RETRY_DELAY_SECONDS));
            
            if (!running_) {
                syslog(LOG_INFO, "[CLIENT A] Client stopped, aborting reconnect");
                break;
            }
            
            try {
                socket_ = boost::asio::ip::tcp::socket(io_);
                connect();
                
                read();
                
                syslog(LOG_INFO, "[CLIENT A] Reconnected successfully on attempt %d!", attempt);
                reconnecting_ = false;
                return;
                
            } catch (const std::exception& e) {
                syslog(LOG_WARNING, "[CLIENT A] Reconnect attempt %d failed: %s", 
                       attempt, e.what());
                
                if (attempt == MAX_RETRIES) {
                    syslog(LOG_ERR, "[CLIENT A] Failed to reconnect after %d attempts - shutting down", 
                           MAX_RETRIES);
                    running_ = false;
                    reconnecting_ = false;
                }
            }
        }
        
        reconnecting_ = false;
    }).detach();
}

void ClientA::handleError(ErrorType type, const std::string& message){
    switch(type) {
        case ErrorType::CONNECTION:
            syslog(LOG_WARNING, "[CLIENT A] Connection error: %s - attempting reconnect", message.c_str());
            handleDisconnect();
            break;
            
        case ErrorType::PROTOCOL:
            syslog(LOG_ERR, "[CLIENT A] Protocol error: %s - stopping client", message.c_str());
            stop(); 
            break;
            
        case ErrorType::INTERNAL:
            syslog(LOG_ERR, "[CLIENT A] Internal error: %s - stopping client", message.c_str());
            stop();
            break;
    }
}

void ClientA::handleDisconnect(){
    if (disconnecting_.exchange(true)) return;
    
    syslog(LOG_INFO, "[CLIENT A] Handling disconnect, attempting to reconnect...");
    
    running_ = false; 

    if (generator_.joinable()) {
        generator_.join();
        syslog(LOG_DEBUG, "[CLIENT A] Generator thread stopped");
    }

    boost::system::error_code ec;
    socket_.close(ec);
    
    std::istream is(&buffer_);
    buffer_.consume(buffer_.size());

    running_ = true;
    
    reconnect();
    
    disconnecting_ = false;
}

void ClientA::read(){
    boost::asio::async_read_until(socket_, buffer_,'\n', 
    [this](const boost::system::error_code& ec, size_t){
        if(!running_) return;

        if(!ec){
            try{
                std::istream is(&buffer_);

                std::string line;
                std::getline(is, line);

                auto json_packet = nlohmann::json::parse(line);
                AccelModule packet = {json_packet["timestamp"], json_packet["module"]};

                std::lock_guard<std::mutex> lg{mutex_};
                auto json_str = Serializer::to_json(packet);
                logger_.write(json_str);
        
                read();
            }catch(const nlohmann::json::parse_error& e){
                syslog(LOG_ERR, "[CLIENT A] JSON parse error (invalid format): %s", e.what());
                handleError(ErrorType::PROTOCOL, "Invalid JSON format from server");
                
            }catch(const std::exception& e){
                syslog(LOG_ERR, "[CLIENT A] Read/parse error: %s", e.what());
                handleError(ErrorType::PROTOCOL, e.what());
            }
        } else if (ec == boost::asio::error::eof ||
                   ec == boost::asio::error::connection_reset ||
                   ec == boost::asio::error::connection_aborted) {
            syslog(LOG_WARNING, "[CLIENT A] Connection error: %s", ec.message().c_str());
            handleError(ErrorType::CONNECTION, ec.message());
        } else if (ec == boost::asio::error::operation_aborted) {
            syslog(LOG_DEBUG, "[CLIENT A] Read operation aborted");
        } else {
            syslog(LOG_WARNING, "[CLIENT A] Unexpected read error: %s", ec.message().c_str());
            handleError(ErrorType::CONNECTION, ec.message());
        }
    });
}

void ClientA::send(){
    try{
        auto json_data = packet_.dump();
        json_data.push_back('\n');
        boost::asio::write(socket_, boost::asio::buffer(json_data.data(), json_data.size()));
    }catch(const boost::system::system_error& e){
        syslog(LOG_ERR, "[CLIENT A] Send network error: %s", e.what());
        handleError(ErrorType::CONNECTION, e.what());
    }catch(const std::exception& e){
        syslog(LOG_ERR, "[CLIENT A] Send error: %s", e.what());
        handleError(ErrorType::INTERNAL, e.what());
    }
}

void ClientA::start_generating(const std::chrono::milliseconds& ms, 
                    std::function<void()> send_callback){
    static SensorEmulator emulator;

    while(running_){
        packet_ = Serializer::to_json(emulator.generate());

        try {
            send_callback();
        } catch (const std::exception& e) {
            syslog(LOG_ERR, "[CLIENT A] Send callback error: %s", e.what());
            stop();
            break;
        }

        std::this_thread::sleep_for(ms);
    }
}

void ClientA::stop(){
    if (!running_) return;
    
    syslog(LOG_INFO, "[CLIENT A] Stopping...");
    running_ = false;
    boost::system::error_code ec;
    socket_.close(ec); 
    if (ec) {
        syslog(LOG_WARNING, "[CLIENT A] Socket close error: %s", ec.message().c_str());
    }
}

void ClientA::join(){
    if(generator_.joinable()){
        generator_.join();
        syslog(LOG_DEBUG, "[CLIENT A] Joined generator thread");
    }
}

void ClientA::start(){
    syslog(LOG_INFO, "[CLIENT A] Starting...");
    connect();

    generator_ = std::thread([this](){
        start_generating(std::chrono::milliseconds(ms_), [this](){
            try{
                send();
            }catch(const std::exception& e) {
                syslog(LOG_ERR, "[CLIENT A] Exception in send callback: %s", e.what());
                stop();
            }
        });
    });

    read();
}