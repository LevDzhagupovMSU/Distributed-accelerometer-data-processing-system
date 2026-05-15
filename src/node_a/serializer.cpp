#include "serializer.hpp"
#include "packet.hpp"

nlohmann::json Serializer::to_json(const AccelPacket& packet){
    nlohmann::json j;

    j["version"] = static_cast<int>(packet.version);
    j["timestamp"] = packet.timestamp;
    j["x"] = packet.x;
    j["y"] = packet.y;
    j["z"] = packet.z;

    return j;
}

nlohmann::json Serializer::to_json(const AccelModule& packet){
    nlohmann::json j;

    j["version"] = static_cast<int>(packet.version);
    j["timestamp"] = packet.timestamp;
    j["module"] = packet.module;

    return j;
}

AccelPacket Serializer::from_json(const nlohmann::json& j){
    AccelPacket packet;
    
    packet.x = 0.0;
    packet.y = 0.0;
    packet.z = 0.0;
    packet.timestamp = 0;
    packet.version = ProtocolVersion::VERSION_1;
    
    if (j.contains("x")) {
        packet.x = j["x"];
    }
    if (j.contains("y")) {
        packet.y = j["y"];
    }
    if (j.contains("z")) {
        packet.z = j["z"];
    }
    if (j.contains("timestamp")) {
        packet.timestamp = j["timestamp"];
    }
    if (j.contains("version")) {
        uint32_t ver = j["version"];
        if (ver != static_cast<int>(ProtocolVersion::VERSION_1)) {
            syslog(LOG_WARNING, "[PACKET] Unknown protocol version: %u, assuming V1.0", ver);
        }
        packet.version = ProtocolVersion::VERSION_1;
    }
    
    return packet;
}

AccelModule Serializer::from_json(const nlohmann::json& j, bool){
    AccelModule packet;
    
    packet.module = 0.0;
    packet.timestamp = 0;
    packet.version = ProtocolVersion::VERSION_1;
    
    if (j.contains("module")) {
        packet.module = j["module"];
    }
    if (j.contains("timestamp")) {
        packet.timestamp = j["timestamp"];
    }
    if (j.contains("version")) {
        uint32_t ver = j["version"];
        if (ver != static_cast<int>(ProtocolVersion::VERSION_1)) {
            syslog(LOG_WARNING, "[PACKET] Unknown protocol version: %u", ver);
        }
        packet.version = ProtocolVersion::VERSION_1;
    }
    
    return packet;
}