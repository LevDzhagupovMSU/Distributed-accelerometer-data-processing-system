#include <gtest/gtest.h>
#include "sensor_emulator.hpp"

TEST(SensorEmulatorTest, GenerateReturnsValidPacket) {
    SensorEmulator emu;
    for (int i = 0; i < 100; ++i) {
        AccelPacket p = emu.generate();
        EXPECT_GE(p.x, -1.1);
        EXPECT_LE(p.x, 1.1);
        EXPECT_GE(p.y, -1.1);
        EXPECT_LE(p.y, 1.1);
        // z = 9.81 + sin(...), sin в [-1..1], значит z в [8.81..10.81]
        EXPECT_GE(p.z, 8.8);
        EXPECT_LE(p.z, 10.82);
        EXPECT_EQ(p.version, ProtocolVersion::VERSION_1);
    }
}