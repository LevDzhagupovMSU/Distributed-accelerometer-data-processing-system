#pragma once

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <mutex>
#include <nlohmann/json.hpp>

class Logger{
    std::ofstream file_;
    std::mutex mutex_;
public:
    Logger(const std::string& path);

    ~Logger();

    void write(const nlohmann::json& json);
    void write(const std::string& text);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
};