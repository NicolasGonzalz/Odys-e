#pragma once

#include "odyssee/net/PacketKind.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace odyssee::net {

/// Numéro d'un joueur connecté : 0 à kMaxPlayers - 1.
using PlayerId = std::uint8_t;

/// Pourquoi le serveur refuse une connexion. Valeurs figées (lues par d'autres versions).
enum class RefuseReason : std::uint8_t {
    WrongVersion = 1,
    ServerFull = 2,
    InvalidName = 3,
    TooManyFromAddress = 4, ///< trop de joueurs déjà connectés depuis cette adresse IP
};

/// Jeton de session : tiré par le serveur pendant la poignée de main (c'est le jeton du défi),
/// connu des deux côtés, puis répété dans chaque paquet échangé une fois connecté. Sans lui, un
/// tiers qui devine l'adresse d'un joueur ne peut ni le déconnecter ni injecter de paquets.
using SessionToken = std::uint64_t;

// --- Les messages de connexion (voir docs/NETWORK.md, « Connexion ») ---------------------------

/// Sur le réseau, la demande est rembourrée jusqu'à kConnectRequestBodySize : la réponse du
/// serveur n'est jamais plus grosse que la demande (pas d'amplification vers une adresse usurpée).
struct ConnectRequest {
    std::string playerName;
    /// Nombre aléatoire choisi par le client pour cette tentative : le défi le répète (le client
    /// ignore un défi qu'il n'a pas demandé) et il rend chaque jeton de session unique.
    std::uint64_t clientSalt = 0;
    friend bool operator==(const ConnectRequest&, const ConnectRequest&) = default;
};
struct Challenge {
    std::uint64_t token = 0;
    std::uint64_t clientSalt = 0;
    friend bool operator==(const Challenge&, const Challenge&) = default;
};
/// Le serveur ne retient rien avant cette réponse : elle répète donc tout ce dont il a besoin
/// (sel et pseudo), que le jeton authentifie.
struct ChallengeResponse {
    std::uint64_t token = 0;
    std::uint64_t clientSalt = 0;
    std::string playerName;
    friend bool operator==(const ChallengeResponse&, const ChallengeResponse&) = default;
};
struct Accepted {
    PlayerId playerId = 0;
    SessionToken sessionToken = 0;
    friend bool operator==(const Accepted&, const Accepted&) = default;
};
struct Refused {
    RefuseReason reason = RefuseReason::WrongVersion;
    friend bool operator==(const Refused&, const Refused&) = default;
};
struct Disconnect {
    SessionToken sessionToken = 0;
    friend bool operator==(const Disconnect&, const Disconnect&) = default;
};
struct KeepAlive {
    SessionToken sessionToken = 0;
    friend bool operator==(const KeepAlive&, const KeepAlive&) = default;
};

/// Taille minimale du contenu d'un ConnectRequest (rembourrage compris). Une demande plus courte
/// est refusée : elle pourrait servir à faire envoyer au serveur plus qu'il n'a reçu.
inline constexpr std::size_t kConnectRequestBodySize = 64;

using ControlMessage = std::variant<ConnectRequest, Challenge, ChallengeResponse, Accepted,
                                    Refused, Disconnect, KeepAlive>;

// --- Encodage ---------------------------------------------------------------------------------

/// Type de paquet correspondant à un message.
[[nodiscard]] PacketKind kindOf(const ControlMessage& message);

/// Contenu (après l'en-tête et le type) d'un message.
[[nodiscard]] std::vector<std::byte> encodeBody(const ControlMessage& message);

/// Relit un message à partir de son type et de son contenu. std::nullopt si invalide.
[[nodiscard]] std::optional<ControlMessage> decodeControl(PacketKind kind,
                                                          std::span<const std::byte> body);

/// Paquet complet (en-tête à la version courante + type + contenu), prêt à être envoyé.
[[nodiscard]] std::vector<std::byte> buildControlPacket(const ControlMessage& message);

/// Texte lisible d'une raison de refus (journaux, interface).
[[nodiscard]] std::string_view describe(RefuseReason reason);

} // namespace odyssee::net
