#pragma once

#include "odyssee/net/Address.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <span>
#include <utility>
#include <vector>

namespace odyssee::net {

/// Mauvaises conditions réseau à imiter (voir docs/NETWORK.md, « Outils de test »).
struct NetworkConditions {
    std::chrono::milliseconds latency{0}; ///< délai ajouté à chaque datagramme (aller simple)
    std::chrono::milliseconds jitter{0};  ///< ± au hasard autour de `latency` (réordonne)
    double lossPercent = 0.0;             ///< 0 à 100 : datagrammes perdus
    double duplicatePercent = 0.0;        ///< 0 à 100 : datagrammes reçus deux fois

    /// Vrai si rien n'est simulé : les datagrammes partent directement.
    [[nodiscard]] bool isPerfect() const;
    friend bool operator==(const NetworkConditions&, const NetworkConditions&) = default;
};

/// Retarde, perd, duplique et réordonne des datagrammes sortants, comme un mauvais réseau.
///
/// Placé devant un socket : l'appelant lui confie ce qu'il voulait envoyer (submit), puis envoie
/// pour de vrai ce qui est « arrivé » (takeDue). Chaque côté simule ses propres envois : pour
/// dégrader les deux sens, on l'active sur le serveur **et** sur le client.
///
/// Le temps est passé en paramètre (`now`) et le hasard vient d'une graine : les tests sont
/// rapides et reproductibles, sur toutes les plateformes.
class NetworkSimulator {
  public:
    using Clock = std::chrono::steady_clock;

    struct Datagram {
        Address to;
        std::vector<std::byte> bytes;
        friend bool operator==(const Datagram&, const Datagram&) = default;
    };

    struct Stats {
        std::uint64_t submitted = 0;  ///< datagrammes confiés
        std::uint64_t dropped = 0;    ///< perdus exprès
        std::uint64_t duplicated = 0; ///< envoyés en double
    };

    /// Les pourcentages sont ramenés entre 0 et 100, les délais négatifs à 0.
    NetworkSimulator(NetworkConditions conditions, std::uint64_t seed);

    /// Confie un datagramme à envoyer : perdu, ou programmé (une ou deux fois) pour plus tard.
    void submit(const Address& to, std::span<const std::byte> bytes, Clock::time_point now);

    /// Retire les datagrammes arrivés à échéance, dans l'ordre de leurs échéances.
    [[nodiscard]] std::vector<Datagram> takeDue(Clock::time_point now);

    /// Prochaine échéance (pour se réveiller à temps), ou rien si aucun datagramme n'attend.
    [[nodiscard]] std::optional<Clock::time_point> nextDue() const;

    [[nodiscard]] std::size_t inFlight() const { return _scheduled.size(); }
    [[nodiscard]] const NetworkConditions& conditions() const { return _conditions; }
    [[nodiscard]] const Stats& stats() const { return _stats; }

  private:
    /// Clé de tri : échéance, puis ordre de dépôt (deux datagrammes à la même échéance gardent
    /// leur ordre d'envoi).
    using Key = std::pair<Clock::time_point, std::uint64_t>;

    [[nodiscard]] bool roll(double percent);
    [[nodiscard]] Clock::duration randomDelay();
    void schedule(const Address& to, std::span<const std::byte> bytes, Clock::time_point now);

    NetworkConditions _conditions;
    std::mt19937_64 _random; ///< générateur entièrement spécifié : mêmes tirages partout
    std::uint64_t _nextOrder = 0;
    std::map<Key, Datagram> _scheduled;
    Stats _stats;
};

} // namespace odyssee::net
