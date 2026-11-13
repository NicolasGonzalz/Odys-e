#include "Vitals.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

constexpr std::int32_t kScale = VitalsTuning::kVitalScale;

std::int32_t drain(std::int32_t value, std::int32_t amount) {
    return std::max(0, value - amount);
}

std::uint8_t toPercent(std::int32_t value) {
    const std::int32_t unitsPerPercent = kScale / net::kMaxVital;
    const std::int32_t clamped = std::clamp(value, 0, kScale);
    return static_cast<std::uint8_t>((clamped + unitsPerPercent - 1) / unitsPerPercent);
}

/// Vie : elle baisse pour chaque besoin vide, remonte si les deux sont bien remplis.
std::int32_t nextHealth(const Vitals& vitals, const VitalsTuning& tuning) {
    const std::int32_t empty = (vitals.hunger == 0 ? 1 : 0) + (vitals.thirst == 0 ? 1 : 0);
    if (empty > 0) {
        return drain(vitals.health, empty * tuning.starvingDamagePerTick);
    }
    const bool wellFed = vitals.hunger > tuning.regenAbove && vitals.thirst > tuning.regenAbove;
    return wellFed ? std::min(kScale, vitals.health + tuning.regenPerTick) : vitals.health;
}

} // namespace

Vitals stepVitals(Vitals vitals, bool sprinting, const VitalsTuning& tuning) {
    const std::int32_t extra = sprinting ? tuning.sprintExtraPerTick : 0;
    vitals.hunger = drain(vitals.hunger, tuning.hungerPerTick + extra);
    vitals.thirst = drain(vitals.thirst, tuning.thirstPerTick + extra);
    vitals.health = nextHealth(vitals, tuning);
    return vitals;
}

net::OwnVitals toOwnVitals(const Vitals& vitals) {
    return {toPercent(vitals.health), toPercent(vitals.hunger), toPercent(vitals.thirst)};
}

} // namespace odyssee::server
