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
};

// --- Les messages de connexion (voir docs/NETWORK.md, « Connexion ») ---------------------------

struct ConnectRequest {
    std::string playerName;
    friend bool operator==(const ConnectRequest&, const ConnectRequest&) = default;
};
struct Challenge {
    std::uint64_t token = 0;
    friend bool operator==(const Challenge&, const Challenge&) = default;
};
struct ChallengeResponse {
    std::uint64_t token = 0;
    friend bool operator==(const ChallengeResponse&, const ChallengeResponse&) = default;
};
struct Accepted {
    PlayerId playerId = 0;
    friend bool operator==(const Accepted&, const Accepted&) = default;
};
struct Refused {
    RefuseReason reason = RefuseReason::WrongVersion;
    friend bool operator==(const Refused&, const Refused&) = default;
};
struct Disconnect {
    friend bool operator==(const Disconnect&, const Disconnect&) = default;
};
struct KeepAlive {
    friend bool operator==(const KeepAlive&, const KeepAlive&) = default;
};

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
