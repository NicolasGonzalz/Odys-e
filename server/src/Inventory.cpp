#include "Inventory.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

std::uint16_t stackOf(data::ItemId item, const data::Catalog& catalog) {
    const data::ItemDef* def = catalog.item(item);
    return def ? def->maxStack : std::uint16_t{0};
}

/// Met ce qui tient dans la case (vide ou du même objet), renvoie le reste.
std::uint16_t fill(net::InventorySlot& slot, data::ItemId item, std::uint16_t count,
                   std::uint16_t maxStack) {
    const std::uint16_t room = static_cast<std::uint16_t>(maxStack - slot.count);
    const std::uint16_t moved = std::min(room, count);
    if (moved > 0) {
        slot.item = item.value;
        slot.count = static_cast<std::uint16_t>(slot.count + moved);
    }
    return static_cast<std::uint16_t>(count - moved);
}

} // namespace

std::uint16_t addItem(Inventory& inventory, data::ItemId item, std::uint16_t count,
                      const data::Catalog& catalog) {
    const std::uint16_t maxStack = stackOf(item, catalog);
    if (maxStack == 0 || count == 0) {
        return count;
    }
    std::uint16_t left = count;
    for (auto& slot : inventory.slots) { // piles du même objet d'abord
        if (left > 0 && slot.count > 0 && slot.item == item.value) {
            left = fill(slot, item, left, maxStack);
        }
    }
    for (auto& slot : inventory.slots) { // puis les cases vides
        if (left > 0 && slot.count == 0) {
            left = fill(slot, item, left, maxStack);
        }
    }
    inventory.changed = inventory.changed || left != count;
    return left;
}

bool removeItem(Inventory& inventory, data::ItemId item, std::uint32_t count) {
    if (countItem(inventory, item) < count) {
        return false;
    }
    std::uint32_t left = count;
    for (auto it = inventory.slots.rbegin(); it != inventory.slots.rend() && left > 0; ++it) {
        if (it->count > 0 && it->item == item.value) { // en partant du sac, pas de la barre
            const auto taken = static_cast<std::uint16_t>(std::min<std::uint32_t>(it->count, left));
            it->count = static_cast<std::uint16_t>(it->count - taken);
            it->item = it->count == 0 ? std::uint16_t{0} : it->item;
            left -= taken;
        }
    }
    inventory.changed = inventory.changed || count > 0;
    return true;
}

std::uint32_t countItem(const Inventory& inventory, data::ItemId item) {
    std::uint32_t total = 0;
    for (const auto& slot : inventory.slots) {
        total += slot.count > 0 && slot.item == item.value ? slot.count : 0U;
    }
    return total;
}

bool moveItem(Inventory& inventory, std::size_t from, std::size_t to,
              const data::Catalog& catalog) {
    if (from >= inventory.slots.size() || to >= inventory.slots.size() || from == to ||
        inventory.slots[from].count == 0) {
        return false;
    }
    net::InventorySlot& source = inventory.slots[from];
    net::InventorySlot& target = inventory.slots[to];
    if (target.count > 0 && target.item == source.item) {
        const data::ItemId item{source.item};
        source.count = fill(target, item, source.count, stackOf(item, catalog));
        source.item = source.count == 0 ? std::uint16_t{0} : source.item;
    } else {
        std::swap(source, target);
    }
    inventory.changed = true;
    return true;
}

net::InventoryContents toContents(const Inventory& inventory) {
    return net::InventoryContents{inventory.slots};
}

} // namespace odyssee::server
