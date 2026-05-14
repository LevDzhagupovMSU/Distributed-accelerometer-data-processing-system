#include "logger.hpp"
#include <mutex>

Logger::Logger(const std::string& path){
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    file_.open(path, std::ios::app);

    if(!file_.is_open()){
        throw std::runtime_error{"Failed to open log file"};
    }
};

Logger::~Logger(){
    std::lock_guard<std::mutex> lg{mutex_};
    file_.close();
};

void Logger::write(const nlohmann::json& json){
    std::lock_guard<std::mutex> lg(mutex_);
    file_ << json.dump() << std::endl;
    file_.flush();
}

void Logger::write(const std::string& text) {
    std::lock_guard<std::mutex> lg(mutex_);
    file_ << text << std::endl;
    file_.flush();
}