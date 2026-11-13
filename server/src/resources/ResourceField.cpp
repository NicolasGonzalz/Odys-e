#include "resources/ResourceField.hpp"

#include "resources/HarvestReach.hpp"

#include <algorithm>
#include <utility>

namespace odyssee::server {

std::size_t ResourceField::spawn(ecs::Registry& registry, std::span<const PlacedResource> placed) {
    std::size_t created = 0;
    for (const PlacedResource& resource : placed) {
        const data::ResourceDef* type = _types ? _types->type(resource.type) : nullptr;
        if (type == nullptr || _nodes.size() >= kMaxNodes) {
            continue;
        }
        const ecs::Entity entity = registry.create();
        registry.add<ResourceNode>(entity, freshNode(*type));
        registry.add<Placement>(entity, Placement{resource.position, resource.yaw});
        const auto id = static_cast<net::EntityId>(net::kFirstResourceEntity + _nodes.size());
        registry.add<NetworkEntity>(entity, NetworkEntity{id});
        _nodes.push_back(entity);
        ++created;
    }
    return created;
}

void ResourceField::stepRegrowth(ecs::Registry& registry) const {
    if (!_types) {
        return;
    }
    registry.each<ResourceNode>([this](ecs::Entity, ResourceNode& node) {
        if (const data::ResourceDef* type = _types->type(node.type)) {
            server::stepRegrowth(node, *type); // la règle (Harvesting.hpp), pas ce système
        }
    });
}

std::optional<ResourceField::Strike> ResourceField::strike(ecs::Registry& registry,
                                                           const std::array<std::int32_t, 3>& feet,
                                                           std::uint16_t yaw, HeldTool tool) const {
    const auto target = findTarget(registry, feet, yaw, tool);
    if (!target) {
        return std::nullopt;
    }
    ResourceNode& node = *registry.get<ResourceNode>(*target);
    const data::ResourceDef& type = *_types->type(node.type);
    const auto harvest = tryHarvest(node, type, tool);
    const auto interval = hitInterval(type, tool);
    if (!harvest || !interval) {
        return std::nullopt; // exclu par findTarget : par prudence
    }
    return Strike{*harvest, *interval, registry.get<NetworkEntity>(*target)->id};
}

void ResourceField::appendNearby(const ecs::Registry& registry,
                                 const std::array<std::int32_t, 3>& around,
                                 std::vector<net::EntityState>& entities, std::size_t limit) const {
    // Les plus proches d'abord ; à distance égale, l'ordre de création (déterministe).
    std::vector<std::pair<std::int64_t, std::size_t>> inRange;
    const std::int64_t radiusSquared = std::int64_t{kSnapshotRadius} * kSnapshotRadius;
    for (std::size_t i = 0; i < _nodes.size(); ++i) {
        const Placement& placement = *registry.get<Placement>(_nodes[i]);
        const std::int64_t distance = horizontalDistanceSquared(around, placement.position);
        if (distance <= radiusSquared) {
            inRange.emplace_back(distance, i);
        }
    }
    std::sort(inRange.begin(), inRange.end());

    for (const auto& [distance, index] : inRange) {
        if (entities.size() >= limit) {
            return;
        }
        const ecs::Entity entity = _nodes[index];
        entities.push_back(toResourceEntity(*registry.get<NetworkEntity>(entity),
                                            *registry.get<Placement>(entity),
                                            *registry.get<ResourceNode>(entity)));
    }
}

std::optional<ecs::Entity> ResourceField::findTarget(const ecs::Registry& registry,
                                                     const std::array<std::int32_t, 3>& feet,
                                                     std::uint16_t yaw, HeldTool tool) const {
    std::optional<ecs::Entity> best;
    std::int64_t bestDistance = 0;
    for (const ecs::Entity entity : _nodes) {
        const Placement& placement = *registry.get<Placement>(entity);
        const std::int64_t distance = horizontalDistanceSquared(feet, placement.position);
        if ((best && distance >= bestDistance) || !canStrike(registry, entity, tool) ||
            !isWithinReach(feet, yaw, placement.position)) {
            continue;
        }
        best = entity;
        bestDistance = distance;
    }
    return best;
}

bool ResourceField::canStrike(const ecs::Registry& registry, ecs::Entity entity,
                              HeldTool tool) const {
    const ResourceNode& node = *registry.get<ResourceNode>(entity);
    const data::ResourceDef* type = _types ? _types->type(node.type) : nullptr;
    return type != nullptr && !node.depleted() && hitInterval(*type, tool).has_value();
}

net::EntityState toResourceEntity(const NetworkEntity& network, const Placement& placement,
                                  const ResourceNode& node) {
    auto flags = static_cast<std::uint8_t>(node.type.value & net::kResourceTypeMask);
    if (node.depleted()) {
        flags = static_cast<std::uint8_t>(flags | net::kResourceDepleted);
    }
    return net::EntityState{network.id, net::EntityKind::Resource, placement.position,
                            placement.yaw, flags};
}

} // namespace odyssee::server
