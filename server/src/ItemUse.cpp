#include "ItemUse.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

const data::ItemDef* itemIn(const Inventory& inventory, std::size_t slot,
                            const data::Catalog& catalog) {
    if (slot >= inventory.slots.size() || inventory.slots[slot].count == 0) {
        return nullptr;
    }
    return catalog.item(data::ItemId{inventory.slots[slot].item});
}

std::int32_t refill(std::int32_t value, std::uint8_t percent) {
    constexpr std::int32_t kUnitsPerPercent = VitalsTuning::kVitalScale / 100;
    return std::min(VitalsTuning::kVitalScale, value + percent * kUnitsPerPercent);
}

} // namespace

std::optional<data::ItemId> heldToolOf(const Inventory& inventory, std::size_t slot,
                                       const data::Catalog& catalog) {
    const data::ItemDef* item = itemIn(inventory, slot, catalog);
    if (item == nullptr || item->category != data::ItemCategory::Tool) {
        return std::nullopt;
    }
    return item->id;
}

bool useItem(Inventory& inventory, std::size_t slot, Vitals& vitals, const data::Catalog& catalog) {
    const data::ItemDef* item = itemIn(inventory, slot, catalog);
    if (item == nullptr || item->category != data::ItemCategory::Consumable ||
        (item->eat == 0 && item->drink == 0)) {
        return false;
    }
    vitals.hunger = refill(vitals.hunger, item->eat);
    vitals.thirst = refill(vitals.thirst, item->drink);
    net::InventorySlot& cell = inventory.slots[slot]; // celui de la case tenue, pas un autre
    cell.count = static_cast<std::uint16_t>(cell.count - 1);
    cell.item = cell.count == 0 ? std::uint16_t{0} : cell.item;
    inventory.changed = true;
    return true;
}

} // namespace odyssee::server
