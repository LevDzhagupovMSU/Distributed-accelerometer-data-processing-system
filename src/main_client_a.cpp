#include "client_a.hpp" 
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <memory>
#include <signal.h>

std::unique_ptr<ClientA> g_clientA;
boost::asio::io_context* g_io = nullptr;

void signal_handler(int signum) {
    std::cout << "\nStopping client A..." << std::endl;
    if (g_clientA) g_clientA->stop();
    if (g_io) g_io->stop();
}

int main() {
    try {
        auto io = std::make_shared<boost::asio::io_context>();
        g_io = io.get();
        
        uint16_t server_port_A = 8080;
        std::string server_ip = "127.0.0.1";
        
        std::cout << "========================================" << std::endl;
        std::cout << "Starting Client A" << std::endl;
        std::cout << "Connecting to port: " << server_port_A << std::endl;
        std::cout << "========================================" << std::endl;
        
        g_clientA = std::make_unique<ClientA>(*io, server_port_A, server_ip);
        
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        g_clientA->start();
        
        std::cout << "Client A started. Sending data to server..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        io->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}