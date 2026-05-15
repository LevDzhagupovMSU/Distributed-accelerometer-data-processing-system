#include <gtest/gtest.h>
#include "logger.hpp"
#include <filesystem>
#include <fstream>

class LoggerTest : public ::testing::Test {
protected:
    std::string test_path = "/tmp/test_logger.log";
    
    void SetUp() override {
        if (std::filesystem::exists(test_path))
            std::filesystem::remove(test_path);
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_path))
            std::filesystem::remove(test_path);
    }
};

TEST_F(LoggerTest, WriteJson) {
    Logger logger(test_path);
    nlohmann::json j = {{"test", 42}};
    logger.write(j);

    std::ifstream file(test_path);
    std::string line;
    std::getline(file, line);
    auto parsed = nlohmann::json::parse(line);
    EXPECT_EQ(parsed["test"], 42);
}

TEST_F(LoggerTest, WriteString) {
    Logger logger(test_path);
    std::string message = "Hello world";
    logger.write(message);  // Явно передаем std::string

    std::ifstream file(test_path);
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "Hello world");
}