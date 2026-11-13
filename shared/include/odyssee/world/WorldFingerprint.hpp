#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace odyssee::world {

/// Empreinte d'un fichier monde (FNV-1a sur 64 bits de tous ses octets) : le serveur l'annonce à
/// l'acceptation d'un joueur, le client la compare à celle de sa propre carte. Deux cartes
/// différentes donneraient une prédiction corrigée sans arrêt ; on le dit au joueur à la place.
///
/// Pas une protection contre la triche : un simple contrôle de version de la carte.
[[nodiscard]] std::uint64_t fingerprintOf(std::span<const std::byte> bytes);

/// Empreinte annoncée par un serveur lancé sans carte (sol plat provisoire).
inline constexpr std::uint64_t kNoMapFingerprint = 0;

} // namespace odyssee::world
