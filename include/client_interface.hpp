#pragma once
#include "error_type.hpp"
#include <string>

class IClient{
public:
    virtual ~IClient() = default;

    virtual void connect() =0;
    virtual void reconnect() =0;
    virtual void read() =0;
    virtual void send() =0;

    virtual void handleError(ErrorType type, const std::string& e) =0;
    virtual void handleDisconnect() =0;
    virtual void stop() =0;
    virtual void join() =0;

    virtual void start() =0;
};