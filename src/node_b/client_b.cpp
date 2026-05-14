#include "client_b.hpp"
#include <boost/asio/read_until.hpp>
#include <exception>
#include <memory>
#include <syslog.h>

ClientB::ClientB(boost::asio::io_context& io, uint16_t port, std::string address, std::chrono::milliseconds ms) : 
                                io_{io}, port_{port}, address_{address}, socket_{io}, ms_{ms} {
    ep_ = std::move(boost::asio::ip::tcp::endpoint{
                        boost::asio::ip::make_address_v4(address_), port_});
    syslog(LOG_INFO, "[CLIENT B] Created, port=%d, address=%s", port, address.c_str());
}; 

ClientB::~ClientB(){
    syslog(LOG_INFO, "[CLIENT B] ~ClientB() called");
    stop();
    join();
}

double ClientB::get_acceleration(double x, double y, double z) const{
    return std::sqrt(std::pow(x,2) + std::pow(y,2) + std::pow(z,2));
}

void ClientB::connect(){
    try{
        socket_.connect(ep_);
        syslog(LOG_INFO, "[CLIENT B] Connected to server at %s:%d", address_.c_str(), port_);
    }catch(const boost::system::system_error& er){
        syslog(LOG_ERR, "[CLIENT B] Connection failed: %s", er.what());
        throw er; 
    }
}

void ClientB::reconnect() {
    if (reconnecting_.exchange(true)) return;
    
    syslog(LOG_INFO, "[CLIENT B] Reconnecting... (max attempts: %d)", MAX_RETRIES);
    
    stop();
    
    std::thread([this]() {
        for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) {

            syslog(LOG_INFO, "[CLIENT B] Reconnect attempt %d/%d", attempt, MAX_RETRIES);
            
            std::this_thread::sleep_for(std::chrono::seconds(RETRY_DELAY_SECONDS));
            
            try {
                start();
                
                syslog(LOG_INFO, "[CLIENT B] Reconnected successfully on attempt %d!", attempt);
                reconnecting_ = false;
                return;
                
            } catch (const std::exception& e) {
                syslog(LOG_WARNING, "[CLIENT B] Reconnect attempt %d failed: %s", 
                       attempt, e.what());
                
                if (attempt == MAX_RETRIES) {
                    syslog(LOG_ERR, "[CLIENT B] Failed to reconnect after %d attempts", 
                           MAX_RETRIES);
                    reconnecting_ = false;
                    stop();
                }
            }
        }
        
        reconnecting_ = false;
    }).detach();
}

void ClientB::handleDisconnect() {
    if (disconnecting_.exchange(true)) return;
    
    syslog(LOG_INFO, "[CLIENT B] Handling disconnect, attempting to reconnect...");
    
    boost::system::error_code ec;
    socket_.close(ec);

    std::istream is(&buffer_);
    buffer_.consume(buffer_.size());
    
    std::lock_guard<std::mutex> lg{mutex_};
    packet_.reset();
    
    reconnect();
    
    disconnecting_ = false;
}

void ClientB::handleError(ErrorType type, const std::string& message) {
    switch(type) {
        case ErrorType::CONNECTION:
            syslog(LOG_WARNING, "[CLIENT B] Connection error: %s - attempting reconnect", message.c_str());
            handleDisconnect();
            break;
            
        case ErrorType::PROTOCOL:
            syslog(LOG_ERR, "[CLIENT B] Protocol error: %s - stopping client", message.c_str());
            stop();
            break;
            
        case ErrorType::INTERNAL:
            syslog(LOG_ERR, "[CLIENT B] Internal error: %s - stopping client", message.c_str());
            stop();
            break;
    }
}

void ClientB::read(){
    boost::asio::async_read_until(socket_, buffer_,'\n', 
    [this](const boost::system::error_code& ec, size_t){
        if(!running_) return;

        if(!ec){
            try{
                std::istream is(&buffer_);

                std::string line;
                std::getline(is, line);

                auto packet = nlohmann::json::parse(line);
        
                std::lock_guard<std::mutex> lg{mutex_};
                auto accel = get_acceleration(packet["x"], packet["y"], packet["z"]);
                packet_ = AccelModule{packet["timestamp"], accel};

                read();
            }catch(const nlohmann::json::parse_error& e){
                syslog(LOG_ERR, "[CLIENT B] JSON parse error (invalid format): %s", e.what());
                handleError(ErrorType::PROTOCOL, "Invalid JSON format from server");
                
            }catch(const std::exception& e){
                syslog(LOG_ERR, "[CLIENT B] Read/parse error: %s", e.what());
                handleError(ErrorType::PROTOCOL, e.what());
            }
        }else if (ec == boost::asio::error::eof ||
                   ec == boost::asio::error::connection_reset ||
                   ec == boost::asio::error::connection_aborted) {
            syslog(LOG_WARNING, "[CLIENT B] Connection error: %s", ec.message().c_str());
            handleError(ErrorType::CONNECTION, ec.message());
        } else if (ec == boost::asio::error::operation_aborted) {
            syslog(LOG_DEBUG, "[CLIENT B] Read operation aborted");
        } else {
            syslog(LOG_WARNING, "[CLIENT B] Unexpected read error: %s", ec.message().c_str());
            handleError(ErrorType::CONNECTION, ec.message());
        }
    });
}

void ClientB::send(){
    auto timer = std::make_shared<boost::asio::steady_timer>(io_);
    timer->expires_after(std::chrono::milliseconds(ms_));
    timer->async_wait([this, timer](const boost::system::error_code& ec){
        if (!running_) return;

        if(ec){
            syslog(LOG_WARNING, "[CLIENT B] Timer error: %s", ec.message().c_str());
            return;
        }

        std::lock_guard<std::mutex> lg{mutex_};
        if(packet_.has_value()){
            auto data = std::make_shared<AccelModule>(packet_.value());
            packet_.reset();
            
            auto json_packet = Serializer::to_json(*data);
            std::string json_str = json_packet.dump() + "\n";
            auto ptr_json_string = std::make_shared<std::string>(std::move(json_str));
            boost::asio::async_write(socket_, boost::asio::buffer(ptr_json_string->data(), ptr_json_string->size()),
            [this, ptr_json_string](const boost::system::error_code& ec, size_t){
                if(!ec) 
                    send();
                else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset || ec == boost::asio::error::connection_aborted) {
                    syslog(LOG_ERR, "[CLIENT B] Send connection error: %s", ec.message().c_str());
                    handleError(ErrorType::CONNECTION, ec.message());
                }else if (ec != boost::asio::error::operation_aborted) {
                    syslog(LOG_ERR, "[CLIENT B] Send error: %s", ec.message().c_str());
                    handleError(ErrorType::CONNECTION, ec.message());
                }
            });
        }else{
            send();
        }
    });
}

void ClientB::stop(){
    if (!running_) return;
    syslog(LOG_INFO, "[CLIENT B] Stopping...");
    running_ = false;
    boost::system::error_code ec;
    socket_.close(ec); 
    if (ec) {
        syslog(LOG_WARNING, "[CLIENT B] Socket close error: %s", ec.message().c_str());
    }
}

void ClientB::join() {}

void ClientB::start(){
    syslog(LOG_INFO, "[CLIENT B] Starting...");
    connect();

    read();
    send();
}