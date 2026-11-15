#include "death/DroppedBags.hpp"

#include "resources/HarvestReach.hpp"
#include "resources/ResourceComponents.hpp"
#include "resources/ResourceField.hpp"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace odyssee::server {

static_assert(net::kFirstResourceEntity + ResourceField::kMaxNodes <= net::kFirstBagEntity,
              "les numéros des sacs ne doivent pas chevaucher ceux des ressources");

namespace {

bool withinReach(const std::array<std::int32_t, 3>& feet, const std::array<std::int32_t, 3>& bag) {
    const std::int64_t radius = DroppedBags::kPickupRadius;
    return horizontalDistanceSquared(feet, bag) <= radius * radius &&
           std::abs(std::int64_t{feet[2]} - bag[2]) <= DroppedBags::kPickupHeight;
}

/// Verse le sac dans l'inventaire, case par case, autant que la place le permet.
bool transfer(BagContents& bag, Inventory& inventory, const data::Catalog& catalog) {
    bool took = false;
    for (net::InventorySlot& slot : bag.slots) {
        if (slot.count == 0) {
            continue;
        }
        const std::uint16_t left = addItem(inventory, data::ItemId{slot.item}, slot.count, catalog);
        took = took || left != slot.count;
        slot = left == 0 ? net::InventorySlot{} : net::InventorySlot{slot.item, left};
    }
    return took;
}

} // namespace

bool BagContents::empty() const {
    return std::all_of(slots.begin(), slots.end(),
                       [](const net::InventorySlot& slot) { return slot.count == 0; });
}

std::optional<net::EntityId> DroppedBags::drop(ecs::Registry& registry,
                                               const std::array<std::int32_t, 3>& feet,
                                               const BagContents& contents) {
    const auto id = freeId(registry);
    if (contents.empty() || !id) {
        return std::nullopt;
    }
    const ecs::Entity bag = registry.create();
    registry.add<BagContents>(bag, contents);
    registry.add<Placement>(bag, Placement{feet, 0});
    registry.add<NetworkEntity>(bag, NetworkEntity{*id});
    _bags.push_back(bag);
    return id;
}

bool DroppedBags::pickUp(ecs::Registry& registry, const std::array<std::int32_t, 3>& feet,
                         Inventory& inventory, const data::Catalog& catalog) {
    bool took = false;
    for (std::size_t i = _bags.size(); i-- > 0;) {
        const ecs::Entity bag = _bags[i];
        const Placement* placement = registry.get<Placement>(bag);
        BagContents* contents = registry.get<BagContents>(bag);
        if (!placement || !contents || !withinReach(feet, placement->position)) {
            continue;
        }
        took = transfer(*contents, inventory, catalog) || took;
        if (contents->empty()) {
            forget(registry, bag);
        }
    }
    return took;
}

void DroppedBags::appendNearby(const ecs::Registry& registry,
                               const std::array<std::int32_t, 3>& around,
                               std::vector<net::EntityState>& entities, std::size_t limit) const {
    std::vector<std::pair<std::int64_t, std::size_t>> inRange;
    const std::int64_t radiusSquared = std::int64_t{kSnapshotRadius} * kSnapshotRadius;
    for (std::size_t i = 0; i < _bags.size(); ++i) {
        const Placement* placement = registry.get<Placement>(_bags[i]);
        if (placement) {
            const std::int64_t distance = horizontalDistanceSquared(around, placement->position);
            if (distance <= radiusSquared) {
                inRange.emplace_back(distance, i);
            }
        }
    }
    std::sort(inRange.begin(), inRange.end());

    for (const auto& [distance, index] : inRange) {
        if (entities.size() >= limit) {
            return;
        }
        const ecs::Entity bag = _bags[index];
        const NetworkEntity* network = registry.get<NetworkEntity>(bag);
        const Placement* placement = registry.get<Placement>(bag);
        const BagContents* contents = registry.get<BagContents>(bag);
        if (network && placement && contents) {
            entities.push_back(net::EntityState{network->id, net::EntityKind::Bag,
                                                placement->position, 0, contents->owner});
        }
    }
}

std::optional<BagContents> DroppedBags::contentsOf(const ecs::Registry& registry,
                                                   net::EntityId id) const {
    for (const ecs::Entity bag : _bags) {
        const NetworkEntity* network = registry.get<NetworkEntity>(bag);
        const BagContents* contents = registry.get<BagContents>(bag);
        if (network && contents && network->id == id) {
            return *contents;
        }
    }
    return std::nullopt;
}

std::optional<net::EntityId> DroppedBags::freeId(const ecs::Registry& registry) const {
    std::array<bool, kMaxBags> used{};
    for (const ecs::Entity bag : _bags) {
        const NetworkEntity* network = registry.get<NetworkEntity>(bag);
        const std::size_t index =
            network ? std::size_t{network->id} - net::kFirstBagEntity : kMaxBags;
        if (index < kMaxBags) {
            used[index] = true;
        }
    }
    const auto free = std::find(used.begin(), used.end(), false);
    if (free == used.end()) {
        return std::nullopt;
    }
    return static_cast<net::EntityId>(net::kFirstBagEntity + (free - used.begin()));
}

void DroppedBags::forget(ecs::Registry& registry, ecs::Entity bag) {
    registry.destroy(bag);
    _bags.erase(std::find(_bags.begin(), _bags.end(), bag));
}

} // namespace odyssee::server
