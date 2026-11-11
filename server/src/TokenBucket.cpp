#include "TokenBucket.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

constexpr std::uint64_t kMilli = 1000;

} // namespace

TokenBucket::TokenBucket(Rate rate, Clock::time_point now)
    : _rate(rate), _milliTokens(rate.burst * kMilli), _lastRefill(now) {}

bool TokenBucket::take(Clock::time_point now) {
    refill(now);
    if (_milliTokens < kMilli) {
        return false;
    }
    _milliTokens -= kMilli;
    return true;
}

bool TokenBucket::isFull(Clock::time_point now) {
    refill(now);
    return _milliTokens >= _rate.burst * kMilli;
}

void TokenBucket::refill(Clock::time_point now) {
    if (now <= _lastRefill) {
        return; // horloge qui n'avance pas (ou recule, dans un test) : rien à ajouter
    }
    // Borné à une heure : largement de quoi remplir le seau, et le produit ne peut pas déborder.
    const auto elapsed = std::min(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastRefill),
        std::chrono::milliseconds{3'600'000});
    // perSecond jetons par seconde = perSecond millièmes de jeton par milliseconde.
    const auto gained = static_cast<std::uint64_t>(elapsed.count()) * _rate.perSecond;
    _milliTokens = std::min(_milliTokens + gained, _rate.burst * kMilli);
    // On n'avance que des millisecondes comptées : la fraction restante n'est pas perdue.
    _lastRefill += elapsed;
}

} // namespace odyssee::server
