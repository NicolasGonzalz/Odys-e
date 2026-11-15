#include "death/Death.hpp"

#include <utility>

namespace odyssee::server {

bool isKeptOnDeath(data::ItemId item, const data::Catalog& catalog) {
    const data::ItemDef* def = catalog.item(item);
    return def && def->category == data::ItemCategory::Equipment;
}

DroppedSlots takeDroppedItems(Inventory& inventory, const data::Catalog& catalog) {
    DroppedSlots dropped{};
    for (std::size_t i = 0; i < inventory.slots.size(); ++i) {
        net::InventorySlot& slot = inventory.slots[i];
        if (slot.count != 0 && !isKeptOnDeath(data::ItemId{slot.item}, catalog)) {
            dropped[i] = std::exchange(slot, net::InventorySlot{});
        }
    }
    inventory.changed = true;
    return dropped;
}

Vitals respawnVitals(const DeathTuning& tuning) {
    return Vitals{VitalsTuning::kVitalScale, tuning.respawnHunger, tuning.respawnThirst};
}

} // namespace odyssee::server
