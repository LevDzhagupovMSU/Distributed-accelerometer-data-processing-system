#pragma once

#include <cstdint>
#include <chrono>
#include <syslog.h>

enum class ProtocolVersion { VERSION_1 = 1, VERSION_2 = 2};

struct AccelPacket{
    uint64_t timestamp;
    double x, y, z;
    ProtocolVersion version = ProtocolVersion::VERSION_1;

    AccelPacket() = default;

    AccelPacket(double x_, double y_, double z_, ProtocolVersion version_);

    uint64_t getTimestamp() const;
};

struct AccelModule{
    uint64_t timestamp;
    double module;
    ProtocolVersion version = ProtocolVersion::VERSION_1;

    AccelModule() = default;

    AccelModule(uint64_t timestamp_, double module_);
};