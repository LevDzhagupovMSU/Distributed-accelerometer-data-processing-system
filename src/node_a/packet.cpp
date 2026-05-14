#include "packet.hpp"

AccelPacket::AccelPacket(double x_, double y_, double z_, ProtocolVersion version_) :
                               x(x_), y(y_), z(z_), version(version_) {
    timestamp = getTimestamp(); 
};

uint64_t AccelPacket::getTimestamp() const{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
}

AccelModule::AccelModule(uint64_t timestamp_, double module_) : 
                                        module(module_),
                                        timestamp(timestamp_) {};