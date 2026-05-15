#include <gtest/gtest.h>
#include "client_b.hpp"
#include <boost/asio/io_context.hpp>

TEST(ClientBTest, CanCreateAndStop) {
    boost::asio::io_context io;
    ClientB client(io, 8081, "127.0.0.1", std::chrono::milliseconds(20));
    client.stop();
    SUCCEED();
}

TEST(ClientBTest, CanCreateWithDifferentFrequency) {
    boost::asio::io_context io;
    ClientB client(io, 8081, "127.0.0.1", std::chrono::milliseconds(10));
    client.stop();
    SUCCEED();
}