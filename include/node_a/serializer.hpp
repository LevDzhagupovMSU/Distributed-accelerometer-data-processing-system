#pragma once

#include "packet.hpp"
#include <nlohmann/json.hpp>

struct Serializer{
    static nlohmann::json to_json(const AccelPacket& packet);
    static nlohmann::json to_json(const AccelModule& packet);

    static AccelPacket from_json(const nlohmann::json& j);
    static AccelModule from_json(const nlohmann::json& j, bool);
};