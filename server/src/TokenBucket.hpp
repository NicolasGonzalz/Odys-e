#pragma once

#include <chrono>
#include <cstdint>

namespace odyssee::server {

/// Seau à jetons : limite un débit tout en tolérant les rafales. Le seau contient au plus `burst`
/// jetons et en regagne `perSecond` chaque seconde ; chaque paquet traité en consomme un. Seau
/// vide : le paquet est écarté.
///
/// Le temps est passé en paramètre (`now`), comme partout : les tests avancent l'horloge.
class TokenBucket {
  public:
    using Clock = std::chrono::steady_clock;

    struct Rate {
        std::uint32_t burst = 0;     ///< rafale tolérée (capacité du seau)
        std::uint32_t perSecond = 0; ///< débit soutenu
    };

    /// Le seau commence plein.
    TokenBucket(Rate rate, Clock::time_point now);

    /// Consomme un jeton. Faux s'il n'y en a plus : le paquet doit être écarté.
    [[nodiscard]] bool take(Clock::time_point now);

    /// Vrai si le seau est (re)devenu plein : personne ne s'en est servi depuis un moment.
    [[nodiscard]] bool isFull(Clock::time_point now);

  private:
    void refill(Clock::time_point now);

    Rate _rate;
    std::uint64_t _milliTokens = 0; ///< millièmes de jeton : le remplissage reste en entiers
    Clock::time_point _lastRefill;
};

} // namespace odyssee::server
