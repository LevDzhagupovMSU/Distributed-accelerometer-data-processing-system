#include "sensor_emulator.hpp"
#include "packet.hpp"
#include <cmath>

SensorEmulator::SensorEmulator() : mt(42), dist(1, 10) {};


AccelPacket SensorEmulator::generate() {
    double x = std::sin(dist(mt));
    double y = std::sin(dist(mt)); 
    double z = 9.81 + std::sin(dist(mt)); 

    return {x,y,z, ProtocolVersion::VERSION_1};
}