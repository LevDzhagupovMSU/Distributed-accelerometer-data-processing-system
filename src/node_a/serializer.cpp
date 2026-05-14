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

AccelPacket from_json(const nlohmann::json& j){
    AccelPacket packet;
    
    if (j.contains("version")) {
        uint32_t ver = j["version"];
        if (ver != static_cast<int>(ProtocolVersion::VERSION_1)) {
            syslog(LOG_WARNING, "[PACKET] Unknown protocol version: %u, assuming V1.0", ver);
        }
        packet.version = ProtocolVersion::VERSION_1;
    } else {
        packet.version = ProtocolVersion::VERSION_1;  
    }
    
    packet.x = j["x"];
    packet.y = j["y"];
    packet.z = j["z"];
    packet.timestamp = j["timestamp"];
    
    return packet;
}

AccelModule Serializer::from_json(const nlohmann::json& j, bool){
    AccelModule packet;
    
    if (j.contains("version")) {
        uint32_t ver = j["version"];
        if (ver != static_cast<int>(ProtocolVersion::VERSION_1)) {
            syslog(LOG_WARNING, "[PACKET] Unknown protocol version: %u", ver);
        }
        packet.version = ProtocolVersion::VERSION_1;
    }
    
    packet.timestamp = j["timestamp"];
    packet.module = j["module"];
    
    return packet;
}