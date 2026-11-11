#pragma once

#include "TokenBucket.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace odyssee::server {

/// Un seau à jetons par adresse IP, pour les expéditeurs qui ne sont pas (encore) des joueurs :
/// une même IP ne peut faire traiter qu'un nombre limité de paquets de poignée de main, quel
/// que soit le nombre de ports qu'elle utilise.
///
/// La mémoire est bornée : au-delà de `maxTracked` adresses suivies, celles dont le seau est
/// redevenu plein sont oubliées ; s'il n'y en a pas (flot d'adresses usurpées), les nouvelles
/// adresses sont refusées le temps que ça passe. Les joueurs connectés ne passent pas par ici.
class IpRateLimiter {
  public:
    using Clock = std::chrono::steady_clock;

    explicit IpRateLimiter(TokenBucket::Rate rate, std::size_t maxTracked = 4096);

    /// Vrai si un paquet de plus venant de `ip` peut être traité maintenant.
    [[nodiscard]] bool allow(std::uint32_t ip, Clock::time_point now);

    [[nodiscard]] std::size_t trackedCount() const { return _buckets.size(); }

  private:
    void forgetIdle(Clock::time_point now);

    TokenBucket::Rate _rate;
    std::size_t _maxTracked;
    std::unordered_map<std::uint32_t, TokenBucket> _buckets;
    Clock::time_point _lastCleanup{};
    bool _hasCleaned = false;
};

} // namespace odyssee::server
