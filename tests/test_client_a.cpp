#include <gtest/gtest.h>
#include "client_a.hpp"
#include <boost/asio/io_context.hpp>

TEST(ClientATest, CanCreateAndStop) {
    boost::asio::io_context io;
    ClientA client(io, 8080, "127.0.0.1", std::chrono::milliseconds(20));
    // Просто проверяем, что объект создаётся и удаляется без ошибок
    client.stop();
    SUCCEED();
}

TEST(ClientATest, CanCreateWithDifferentFrequency) {
    boost::asio::io_context io;
    ClientA client(io, 8080, "127.0.0.1", std::chrono::milliseconds(10));
    client.stop();
    SUCCEED();
}