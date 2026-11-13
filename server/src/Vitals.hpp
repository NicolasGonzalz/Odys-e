#pragma once

#include "odyssee/net/GameMessages.hpp"

#include <cstdint>

namespace odyssee::server {

/// Réglages des besoins vitaux (T051), provisoires : à ajuster en jouant (docs/GAMEPLAY.md §1).
///
/// Unités : chaque besoin va de 0 à kVitalScale (100 000) ; le pourcentage montré au joueur en
/// est le millième. Les baisses sont données par tick (20 ticks par seconde).
struct VitalsTuning {
    static constexpr std::int32_t kVitalScale = 100'000;

    std::int32_t hungerPerTick = 4;          ///< rassasié → affamé en ~21 min
    std::int32_t thirstPerTick = 6;          ///< désaltéré → assoiffé en ~14 min
    std::int32_t sprintExtraPerTick = 4;     ///< courir creuse la faim et la soif deux fois plus
    std::int32_t starvingDamagePerTick = 25; ///< à 0 de faim ou de soif : -100 % de vie en ~3 min
    std::int32_t regenPerTick = 15;   ///< bien nourri et désaltéré : +100 % de vie en ~5,5 min
    std::int32_t regenAbove = 60'000; ///< faim et soif au-dessus de 60 % pour récupérer
};

inline constexpr VitalsTuning kDefaultVitalsTuning{};

/// Composant : vie, faim et soif d'une entité vivante (les joueurs ; les créatures plus tard).
struct Vitals {
    std::int32_t health = VitalsTuning::kVitalScale;
    std::int32_t hunger = VitalsTuning::kVitalScale; ///< kVitalScale = rassasié
    std::int32_t thirst = VitalsTuning::kVitalScale;
};

/// Un tick de besoins : la faim et la soif baissent (plus vite en courant) ; à zéro, la vie
/// baisse ; bien nourri, elle remonte. Tout reste entre 0 et kVitalScale. Déterministe.
[[nodiscard]] Vitals stepVitals(Vitals vitals, bool sprinting,
                                const VitalsTuning& tuning = kDefaultVitalsTuning);

/// Ce que le joueur voit, en pourcentage (arrondi vers le haut : 0 % veut dire vraiment vide).
[[nodiscard]] net::OwnVitals toOwnVitals(const Vitals& vitals);

} // namespace odyssee::server
