#pragma once

#include "odyssee/net/Sequence.hpp"

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>

namespace odyssee::client {

/// Ce que le HUD réseau affiche du trafic (T034), mesuré sur la dernière seconde.
struct TrafficStats {
    double sentKilobytesPerSecond = 0.0;
    double receivedKilobytesPerSecond = 0.0;
    double incomingLossPercent = 0.0; ///< paquets du serveur jamais arrivés
};

/// Mesure le débit montant et descendant, et les pertes à la réception, sur une fenêtre glissante
/// d'une seconde.
///
/// Pertes : le serveur numérote ses paquets ; sur la fenêtre, on compare le nombre de numéros
/// reçus à l'étendue des numéros (du plus ancien au plus récent). Un paquet arrivé en retard
/// compte comme reçu ; un doublon ne compte qu'une fois.
class TrafficMeter {
  public:
    using Clock = std::chrono::steady_clock;
    static constexpr Clock::duration kWindow = std::chrono::seconds{1};

    void onSent(std::size_t bytes, Clock::time_point now);
    void onReceived(std::size_t bytes, Clock::time_point now);
    /// Un paquet de jeu du serveur, de numéro `sequence`, est arrivé.
    void onGamePacket(net::Sequence sequence, Clock::time_point now);

    [[nodiscard]] TrafficStats stats(Clock::time_point now) const;

  private:
    struct Event {
        Clock::time_point at;
        std::size_t bytes = 0;
    };
    struct Arrival {
        Clock::time_point at;
        net::Sequence sequence = 0;
    };

    void forgetOld(Clock::time_point now);
    [[nodiscard]] double lossPercent(Clock::time_point now) const;

    std::deque<Event> _sent;
    std::deque<Event> _received;
    std::deque<Arrival> _arrivals;
};

} // namespace odyssee::client
