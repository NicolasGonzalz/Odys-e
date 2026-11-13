#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace odyssee {

/// Version du protocole réseau. Incrémentée à chaque changement incompatible :
/// un client et un serveur de versions différentes refusent de se connecter.
/// 2 : paquets de jeu (T024). 3 : poignée de main sans état, jeton de session.
/// 4 : code d'invitation dans la réponse au défi (T056). 5 : besoins vitaux dans le snapshot (T051).
/// 6 : empreinte de la carte dans l'acceptation. 7 : entités ressources dans le snapshot (T044).
inline constexpr std::uint16_t kProtocolVersion = 7;

/// Fréquence de simulation du serveur (ticks par seconde).
inline constexpr std::uint32_t kTickRate = 20;

/// Durée fixe d'un tick de simulation.
inline constexpr std::chrono::nanoseconds kTickDuration{1'000'000'000 / kTickRate};

/// Port UDP par défaut du serveur.
inline constexpr std::uint16_t kDefaultPort = 27015;

/// Taille maximale d'un datagramme applicatif (sous la MTU usuelle, pour éviter la
/// fragmentation IP).
inline constexpr std::size_t kMaxPacketSize = 1200;

// --- Connexion (T023) -------------------------------------------------------------------------

/// Joueurs par monde : un par survivant (docs/CHARACTERS.md).
inline constexpr std::size_t kMaxPlayers = 4;

/// Longueur maximale d'un pseudo, en octets UTF-8.
inline constexpr std::size_t kMaxPlayerNameLength = 16;

/// Sans nouvelles d'un joueur connecté pendant ce délai, il est déconnecté.
inline constexpr std::chrono::seconds kConnectionTimeout{10};

/// Durée maximale d'une poignée de main (demande → défi → acceptation).
inline constexpr std::chrono::seconds kHandshakeTimeout{5};

/// Intervalle de renvoi des messages de poignée de main (ils peuvent se perdre).
inline constexpr std::chrono::milliseconds kHandshakeResendInterval{250};

/// Sans autre trafic, chaque côté envoie un KeepAlive à ce rythme pour ne pas être jugé parti.
inline constexpr std::chrono::seconds kKeepAliveInterval{1};

// --- Messages de jeu (T024) -------------------------------------------------------------------

/// Inputs envoyés dans chaque paquet : celui du tick et les précédents (0,8 s à 20 ticks/s).
/// Une perte isolée ne coûte rien : l'input perdu revient dans les paquets suivants.
inline constexpr std::size_t kMaxInputsPerBatch = 16;

/// Entités au plus dans un snapshot (il doit tenir dans un paquet). La zone d'intérêt (M3)
/// choisira lesquelles envoyer ; au-delà, il faudra la compression delta.
inline constexpr std::size_t kMaxSnapshotEntities = 60;

} // namespace odyssee
