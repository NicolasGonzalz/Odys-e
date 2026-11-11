#include "odyssee/net/ReceivedPackets.hpp"

namespace odyssee::net {

namespace {

constexpr std::uint16_t kWindow = 32; // nombre de paquets précédents couverts par ackBits

/// Bit correspondant au paquet situé `distance` paquets avant le plus récent (distance ≥ 1).
constexpr std::uint32_t bitFor(std::uint16_t distance) { return 1u << (distance - 1); }

} // namespace

Arrival ReceivedPackets::record(Sequence sequence) {
    if (!_hasReceived) {
        _hasReceived = true;
        _latest = sequence;
        _bits = 0;
        return Arrival::New;
    }

    if (sequence == _latest) {
        return Arrival::Duplicate;
    }
    return isMoreRecent(sequence, _latest) ? recordNewer(sequence) : recordOlder(sequence);
}

Arrival ReceivedPackets::recordNewer(Sequence sequence) {
    // Le nouveau paquet devient le plus récent : l'historique glisse de `shift` cases,
    // et l'ancien « plus récent » prend la case shift - 1.
    const std::uint16_t shift = sequenceDistance(sequence, _latest);
    if (shift > kMaxJump) {
        return Arrival::TooFarAhead;
    }

    if (shift > kWindow) {
        _bits = 0; // tout l'historique sort de la fenêtre
    } else if (shift == kWindow) {
        _bits = bitFor(kWindow); // décaler de 32 bits serait un comportement indéfini en C++
    } else {
        _bits = (_bits << shift) | bitFor(shift);
    }

    _latest = sequence;
    return Arrival::New;
}

Arrival ReceivedPackets::recordOlder(Sequence sequence) {
    const std::uint16_t distance = sequenceDistance(_latest, sequence);
    if (distance > kWindow) {
        return Arrival::TooOld;
    }

    const std::uint32_t bit = bitFor(distance);
    if ((_bits & bit) != 0) {
        return Arrival::Duplicate;
    }
    _bits |= bit;
    return Arrival::New;
}

} // namespace odyssee::net
