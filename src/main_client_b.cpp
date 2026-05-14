#include "client_b.hpp" 
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <memory>
#include <signal.h>

std::unique_ptr<ClientB> g_clientB;
boost::asio::io_context* g_io = nullptr;

void signal_handler(int signum) {
    std::cout << "\nStopping client B..." << std::endl;
    if (g_clientB) g_clientB->stop();
    if (g_io) g_io->stop();
}

int main() {
    try {
        auto io = std::make_shared<boost::asio::io_context>();
        g_io = io.get();
        
        uint16_t server_port_B = 8081;
        std::string server_ip = "127.0.0.1";
        
        std::cout << "========================================" << std::endl;
        std::cout << "Starting Client B" << std::endl;
        std::cout << "Connecting to port: " << server_port_B << std::endl;
        std::cout << "========================================" << std::endl;
        
        g_clientB = std::make_unique<ClientB>(*io, server_port_B, server_ip);
        
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        g_clientB->start();
        
        std::cout << "Client B started. Waiting for data from server..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        io->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}