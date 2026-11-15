#pragma once

#include "Inventory.hpp"
#include "Vitals.hpp"

#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <array>
#include <cstdint>

namespace odyssee::server {

/// Règles de la mort (T052, docs/VISION.md « Sac au sol ») : l'inventaire reste sur place, les
/// objets équipés (sac à dos, carte, journal, gourde) suivent le joueur, qui réapparaît au point
/// de repos.

/// Ce que le joueur a en revenant : provisoire, à ajuster en jouant.
struct DeathTuning {
    std::int32_t respawnHunger = VitalsTuning::kVitalScale / 2;
    std::int32_t respawnThirst = VitalsTuning::kVitalScale / 2;
};

inline constexpr DeathTuning kDefaultDeathTuning{};

/// Les cases qui tombent dans le sac.
using DroppedSlots = std::array<net::InventorySlot, net::kInventorySlots>;

/// Vrai si l'objet reste au joueur quand il meurt (catégorie « équipement »).
[[nodiscard]] bool isKeptOnDeath(data::ItemId item, const data::Catalog& catalog);

/// Vide `inventory` de ce qui tombe à la mort et le renvoie, case par case (les objets gardés
/// restent à leur place). L'inventaire est marqué à renvoyer au joueur.
[[nodiscard]] DroppedSlots takeDroppedItems(Inventory& inventory, const data::Catalog& catalog);

/// Besoins vitaux au retour : pleine vie, faim et soif à moitié.
[[nodiscard]] Vitals respawnVitals(const DeathTuning& tuning = kDefaultDeathTuning);

} // namespace odyssee::server
