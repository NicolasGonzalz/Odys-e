#pragma once

#include <cstdint>

namespace odyssee::net {

/// Numéro de paquet sur 16 bits. Il reboucle : après 65535 vient 0.
using Sequence = std::uint16_t;

/// Écart « vers l'avant » de `older` à `newer`, modulo 65536 (ex. de 65534 à 2 : 4).
constexpr std::uint16_t sequenceDistance(Sequence newer, Sequence older) {
    return static_cast<std::uint16_t>(newer - older);
}

/// Vrai si `a` est plus récent que `b`, en tenant compte du rebouclage :
/// `a` est plus récent si on l'atteint depuis `b` en avançant de moins d'un demi-tour.
constexpr bool isMoreRecent(Sequence a, Sequence b) {
    const std::uint16_t distance = sequenceDistance(a, b);
    return distance != 0 && distance < 0x8000;
}

} // namespace odyssee::net
