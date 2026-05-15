#include <gtest/gtest.h>
#include "packet.hpp"
#include <chrono>

TEST(AccelPacketTest, ConstructorAndGetters) {
    double x = 1.0, y = 2.0, z = 3.0;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    AccelPacket p(x, y, z, ProtocolVersion::VERSION_1);

    EXPECT_DOUBLE_EQ(p.x, x);
    EXPECT_DOUBLE_EQ(p.y, y);
    EXPECT_DOUBLE_EQ(p.z, z);
    EXPECT_EQ(p.version, ProtocolVersion::VERSION_1);
    EXPECT_GE(p.timestamp, now - 100); // timestamp не может сильно отличаться
}

TEST(AccelPacketTest, TimestampIsRecent) {
    AccelPacket p(0, 0, 0, ProtocolVersion::VERSION_1);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    EXPECT_LE(p.timestamp, now);
    EXPECT_GE(p.timestamp, now - 100);
}

TEST(AccelModuleTest, Constructor) {
    uint64_t ts = 12345678;
    double mod = 9.81;
    AccelModule m(ts, mod);
    EXPECT_EQ(m.timestamp, ts);
    EXPECT_DOUBLE_EQ(m.module, mod);
    EXPECT_EQ(m.version, ProtocolVersion::VERSION_1);
}