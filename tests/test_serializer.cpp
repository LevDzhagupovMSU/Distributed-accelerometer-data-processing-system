#include <gtest/gtest.h>
#include "serializer.hpp"

TEST(SerializerTest, AccelPacketRoundTrip) {
    AccelPacket original(1.1, 2.2, 9.81, ProtocolVersion::VERSION_1);
    original.timestamp = 1234567890;

    nlohmann::json j = Serializer::to_json(original);
    
    Serializer serializer;
    AccelPacket deserialized = serializer.from_json(j);

    EXPECT_DOUBLE_EQ(deserialized.x, original.x);
    EXPECT_DOUBLE_EQ(deserialized.y, original.y);
    EXPECT_DOUBLE_EQ(deserialized.z, original.z);
    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_EQ(deserialized.version, original.version);
}

TEST(SerializerTest, AccelModuleRoundTrip) {
    AccelModule original(123456, 9.81);
    nlohmann::json j = Serializer::to_json(original);
    
    Serializer serializer;
    AccelModule deserialized = serializer.from_json(j, true);

    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_DOUBLE_EQ(deserialized.module, original.module);
    EXPECT_EQ(deserialized.version, original.version);
}

TEST(SerializerTest, FromJsonMissingVersion) {
    nlohmann::json j = {
        {"timestamp", 999},
        {"x", 0.5},
        {"y", 0.6},
        {"z", 9.8}
    };
    
    AccelPacket p = Serializer::from_json(j);
    EXPECT_EQ(p.version, ProtocolVersion::VERSION_1);
}

TEST(SerializerTest, FromJsonMissingFields) {
    nlohmann::json j = {
        {"x", 1.0},
        {"y", 2.0}
        // нет timestamp и z
    };
    
    AccelPacket p = Serializer::from_json(j);
    EXPECT_DOUBLE_EQ(p.x, 1.0);
    EXPECT_DOUBLE_EQ(p.y, 2.0);
    EXPECT_DOUBLE_EQ(p.z, 0.0); // значение по умолчанию
    EXPECT_EQ(p.timestamp, 0);  // значение по умолчанию
}