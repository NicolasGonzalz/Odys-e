#pragma once

#include <chrono>
#include <cstdint>

namespace odyssee {

/// Version du protocole réseau. Incrémentée à chaque changement incompatible :
/// un client et un serveur de versions différentes refusent de se connecter.
inline constexpr std::uint16_t kProtocolVersion = 1;

/// Fréquence de simulation du serveur (ticks par seconde).
inline constexpr std::uint32_t kTickRate = 20;

/// Durée fixe d'un tick de simulation.
inline constexpr std::chrono::nanoseconds kTickDuration{1'000'000'000 / kTickRate};

/// Port UDP par défaut du serveur.
inline constexpr std::uint16_t kDefaultPort = 27015;

/// Taille maximale d'un datagramme applicatif (sous la MTU usuelle pour éviter la fragmentation IP).
inline constexpr std::size_t kMaxPacketSize = 1200;

} // namespace odyssee
