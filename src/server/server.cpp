#include "server.hpp"
#include "session.hpp"
#include <sys/syslog.h>

Server::Server(boost::asio::io_context& io,
               uint16_t port_A, uint16_t port_B,
               const std::string& address) : io_{io}, address_{address},
                                             acceptor_A_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(address), port_A)),
                                             acceptor_B_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(address), port_B)) {
    syslog(LOG_INFO, "[SERVER] Server created on %s:%d (A) and %d (B)", address.c_str(), port_A, port_B);
};

Server::~Server(){
    syslog(LOG_INFO, "[SERVER] ~Server() called");
    stop();
};

void Server::start(){
    syslog(LOG_INFO, "[SERVER] Starting server...");
    accept_A();
    accept_B();
}

void Server::accept_A(){
    acceptor_A_.async_accept([this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket){
        if(ec){
            syslog(LOG_WARNING, "[SERVER] accept_A error: %s", ec.message().c_str());
            accept_A();
            return;
        }

        std::string client_ip = socket.remote_endpoint().address().to_string();
        uint16_t client_port = socket.remote_endpoint().port();
        
        syslog(LOG_INFO, "[SERVER] Client A connected from %s:%d", client_ip.c_str(), client_port);

        waiting_A_.push(std::move(socket));

        create_session();

        accept_A();
    });
}

void Server::accept_B(){
    acceptor_B_.async_accept([this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket){
        if(ec){
            syslog(LOG_WARNING, "[SERVER] accept_B error: %s", ec.message().c_str());
            accept_B();
            return;
        }

        std::string client_ip = socket.remote_endpoint().address().to_string();
        uint16_t client_port = socket.remote_endpoint().port();
        
        syslog(LOG_INFO, "[SERVER] Client B connected from %s:%d", client_ip.c_str(), client_port);

        waiting_B_.push(std::move(socket));

        create_session();

        accept_B();
    });
}

void Server::create_session(){
    if(!waiting_A_.empty() && !waiting_B_.empty()){
        auto session = std::make_shared<Session>(
            std::move(waiting_A_.front()), std::move(waiting_B_.front()),
            [this](std::shared_ptr<Session> s,const SocketType socket_type, std::optional<boost::asio::ip::tcp::socket> socket){
                remove_session(s, socket_type, std::move(socket));
            });

        session->start();

        waiting_A_.pop();
        waiting_B_.pop();

        sessions_.push_back(session);

        syslog(LOG_INFO, "[SERVER] Session created. Total sessions now: %zu", sessions_.size());
        return;
    }

    if(!waiting_A_.empty() && waiting_B_.empty()){
        syslog(LOG_DEBUG, "[SERVER] Waiting for client B... (A queue size: %zu)", waiting_A_.size());
        return;
    }

    if(waiting_A_.empty() && !waiting_B_.empty()){
        syslog(LOG_DEBUG, "[SERVER] Waiting for client A... (B queue size: %zu)", waiting_B_.size());
        return;
    }
}

void Server::remove_session(std::shared_ptr<Session> s, SocketType socket_type, std::optional<boost::asio::ip::tcp::socket> socket){
    switch (socket_type){
        case SocketType::ALL:
            syslog(LOG_INFO, "[SERVER] Closing both sockets, no return to queue");
        break;
        
        case SocketType::SOCKET_A:
            if (socket.has_value() && socket->is_open()) {
                waiting_A_.push(std::move(socket.value()));
                syslog(LOG_INFO, "[SERVER] Socket A returned to queue. Queue size: %zu", 
                       waiting_A_.size());
            } else {
                syslog(LOG_WARNING, "[SERVER] Attempted to return invalid socket A");
            }
        break;

        case SocketType::SOCKET_B:
            if (socket.has_value() && socket->is_open()) {
                waiting_B_.push(std::move(socket.value()));
                syslog(LOG_INFO, "[SERVER] Socket B returned to queue. Queue size: %zu", 
                       waiting_B_.size());
            } else {
                syslog(LOG_WARNING, "[SERVER] Attempted to return invalid socket B");
            }
        break;   
    }

    auto it = std::find(sessions_.begin(), sessions_.end(), s);
    if (it != sessions_.end()) {
        sessions_.erase(it);
        it->get()->stop();
        syslog(LOG_INFO, "[SERVER] Session removed. Remaining sessions: %zu", sessions_.size());
    }else {
        syslog(LOG_WARNING, "[SERVER] Attempted to remove non-existent session");
    }

    create_session();
}

void Server::stop(){
    syslog(LOG_INFO, "[SERVER] Stopping server...");

    syslog(LOG_DEBUG, "[SERVER] Stopping %zu active sessions", sessions_.size());
    for (auto& session : sessions_) {
        if (session) session->stop();
    }
    sessions_.clear();

    size_t a_queue_size = waiting_A_.size();
    size_t b_queue_size = waiting_B_.size();

    while (!waiting_A_.empty()) waiting_A_.pop();
    while (!waiting_B_.empty()) waiting_B_.pop();

    syslog(LOG_INFO, "[SERVER] Server stopped. Cleared A queue (%zu), B queue (%zu)", a_queue_size, b_queue_size);
}