#include "resources/Harvesting.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

bool isListedTool(const data::ResourceDef& type, HeldTool tool) {
    return tool && std::find(type.tools.begin(), type.tools.end(), *tool) != type.tools.end();
}

} // namespace

ResourceNode freshNode(const data::ResourceDef& type) {
    return ResourceNode{type.id, type.hits, 0};
}

std::optional<std::uint32_t> hitInterval(const data::ResourceDef& type, HeldTool tool) {
    if (type.tools.empty() || isListedTool(type, tool)) {
        return type.hitTicks;
    }
    if (!type.handAllowed) {
        return std::nullopt;
    }
    return type.hitTicks * data::kBareHandSlowdown;
}

std::optional<Harvest> tryHarvest(ResourceNode& node, const data::ResourceDef& type,
                                  HeldTool tool) {
    if (node.depleted() || !hitInterval(type, tool)) {
        return std::nullopt;
    }

    --node.hitsLeft;
    if (node.depleted()) {
        node.regrowTicksLeft = type.regrowTicks;
    }
    return Harvest{type.yield.item, type.yield.count};
}

void stepRegrowth(ResourceNode& node, const data::ResourceDef& type) {
    if (!node.depleted()) {
        return;
    }
    if (node.regrowTicksLeft > 0) {
        --node.regrowTicksLeft;
    }
    if (node.regrowTicksLeft == 0) {
        node.hitsLeft = type.hits;
    }
}

} // namespace odyssee::server
