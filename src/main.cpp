#include "server.hpp"
#include "client_a.hpp"
#include "client_b.hpp"
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>

std::unique_ptr<Server> g_server;
std::vector<std::unique_ptr<ClientA>> g_clientsA;
std::vector<std::unique_ptr<ClientB>> g_clientsB;
boost::asio::io_context* g_io = nullptr;
std::atomic<bool> g_shutting_down{false};

void signal_handler(int signum) {
    if (g_shutting_down.exchange(true)) return;
    
    syslog(LOG_INFO, "Received signal %d, shutting down...", signum);
    
    if (g_server) {
        g_server->stop();
    }
    
    for (auto& client : g_clientsA) {
        if (client) client->stop();
    }
    
    for (auto& client : g_clientsB) {
        if (client) client->stop();
    }
    
    if (g_io) {
        g_io->stop();
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);  
    }
    
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    chdir("/");
    
    umask(0);
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    open("/dev/null", O_RDWR); 
    dup(STDIN_FILENO);        
    dup(STDIN_FILENO);        
}

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "Options:\n"
              << "  --daemon       Запустить демона\n"
              << "  --foreground   Запустить в обычном режиме\n"
              << "  --help         Помощь\n"
              << "  --clients N    Число пар клиентов(A и B) (дефолт: 1 пара)\n"
              << "  --port-a N     Порт для клиента А (дефолт: 8080)\n"
              << "  --port-b N     Порт для клиента А (дефолт: 8081)\n"
              << "  --address IP   Адресс Сервера (дефолт: 127.0.0.1)\n"
              << "  --frequency    Частота сенсора в Гц (дефолт: 50)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    bool daemon_mode = false;
    bool foreground_mode = true;
    int num_clients = 1;
    uint16_t port_a = 8080;
    uint16_t port_b = 8081;
    std::string server_address = "127.0.0.1";
    std::chrono::milliseconds frequency = std::chrono::milliseconds(20);
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--daemon") {
            daemon_mode = true;
            foreground_mode = false;
        }else if (arg == "--foreground") {
            daemon_mode = false;
            foreground_mode = true;
        }else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }else if (arg == "--clients" && i + 1 < argc) {
            num_clients = std::atoi(argv[++i]);
        }else if (arg == "--port-a" && i + 1 < argc) {
            port_a = static_cast<uint16_t>(std::atoi(argv[++i]));
        }else if (arg == "--port-b" && i + 1 < argc) {
            port_b = static_cast<uint16_t>(std::atoi(argv[++i]));
        }else if (arg == "--address" && i + 1 < argc) {
            server_address = argv[++i];
        }else if(arg == "--frequency" && i + 1 < argc){
            int freq_hz = std::atoi(argv[++i]);
            if (freq_hz > 0) {
                frequency = std::chrono::milliseconds(1000 / freq_hz);
            } else {
                std::cout << "Error: frequency must be positive" << std::endl;
                return 1;
            }
        }
    }
    
    if (daemon_mode) {
        daemonize();
        openlog("task2", LOG_PID | LOG_CONS, LOG_DAEMON);
        syslog(LOG_INFO, "Daemon started, PID=%d", getpid());
        syslog(LOG_INFO, "Config: clients=%d, ports=%d/%d, address=%s",
               num_clients, port_a, port_b, server_address.c_str());
    } else {
        openlog("task2", LOG_PID | LOG_CONS | LOG_PERROR, LOG_DAEMON);
        syslog(LOG_INFO, "Application started in foreground mode");
    }
    
    try {
        auto io = std::make_shared<boost::asio::io_context>();
        g_io = io.get();
        
        g_server = std::make_unique<Server>(*io, port_a, port_b, server_address);
        g_server->start();
        syslog(LOG_INFO, "Server started on %s:%d (A) and %d (B)",
               server_address.c_str(), port_a, port_b);
        
        syslog(LOG_INFO, "Creating %d client pairs", num_clients);
        for (int i = 0; i < num_clients; ++i) {
            g_clientsA.push_back(std::make_unique<ClientA>(*io, port_a, server_address, frequency));
            g_clientsB.push_back(std::make_unique<ClientB>(*io, port_b, server_address, frequency));
        }
        
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGHUP, signal_handler);
        std::signal(SIGPIPE, SIG_IGN);
        
        for (auto& client : g_clientsA) {
            client->start();
        }
        for (auto& client : g_clientsB) {
            client->start();
        }

        io->run();
        
        if (foreground_mode) {
            std::cout << "Application stopped" << std::endl;
        }
        
        syslog(LOG_INFO, "Application stopped normally");
        
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Fatal error: %s", e.what());
        if (foreground_mode) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        closelog();
        return 1;
    }
    
    closelog();
    return 0;
}