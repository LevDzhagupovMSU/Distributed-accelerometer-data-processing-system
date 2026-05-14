#pragma once

#include "packet.hpp"
#include <cmath>
#include <random>

class SensorEmulator{
    std::mt19937 mt;
    std::uniform_int_distribution<int> dist;
public:
    SensorEmulator();

    AccelPacket generate();
};