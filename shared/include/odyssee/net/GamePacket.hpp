#pragma once

#include "odyssee/net/PacketHeader.hpp"
#include "odyssee/net/ReliableMessage.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace odyssee::net {

/// Message non fiable : envoyé une fois, jamais renvoyé (inputs, snapshots). `type` est un
/// GameMessageType, gardé brut : un type inconnu se lit sans casser le reste du paquet.
struct UnreliableMessage {
    std::uint8_t type = 0;
    std::vector<std::byte> data;

    friend bool operator==(const UnreliableMessage&, const UnreliableMessage&) = default;
};

/// Contenu d'un paquet de jeu (PacketKind::Game), après l'en-tête et le type :
///
///   jeton de session (u64)
///   | messages fiables (ReliableCodec) | count (u8) | [type (u8) | size (u16) | data…] × count
///
/// Le jeton de session prouve que le paquet vient bien de l'autre côté de la connexion (voir
/// ControlMessages.hpp) : il est vérifié avant tout le reste.
/// Les numéros `sequence` / `ack` / `ackBits` de l'en-tête servent ici : acquittements des
/// paquets, et donc renvoi des messages fiables (voir ReliableSender).
struct GamePacketBody {
    std::vector<ReliableMessage> reliable;
    std::vector<UnreliableMessage> unreliable;
    std::uint64_t sessionToken = 0;

    friend bool operator==(const GamePacketBody&, const GamePacketBody&) = default;
};

namespace game_packet {

inline constexpr std::size_t kSessionTokenSize = 8;
inline constexpr std::size_t kUnreliableListOverhead = 1; ///< le compteur
inline constexpr std::size_t kUnreliableOverhead = 3;     ///< type + size
inline constexpr std::size_t kMaxUnreliablePerPacket = 32;

/// Place occupée par un message non fiable une fois encodé.
[[nodiscard]] constexpr std::size_t encodedSize(const UnreliableMessage& message) {
    return kUnreliableOverhead + message.data.size();
}

/// Taille du paquet complet (en-tête, type et contenu) : l'appelant vérifie qu'elle ne dépasse
/// pas kMaxPacketSize avant d'envoyer.
[[nodiscard]] std::size_t packetSize(const GamePacketBody& body);

/// Paquet complet prêt à être envoyé. Au-delà de kMaxUnreliablePerPacket, les messages non
/// fiables en trop ne partent pas.
[[nodiscard]] std::vector<std::byte> build(const PacketHeader& header, const GamePacketBody& body);

/// Relit le contenu d'un paquet de jeu. std::nullopt si tronqué, incohérent ou trop long.
[[nodiscard]] std::optional<GamePacketBody> parse(std::span<const std::byte> body);

} // namespace game_packet

} // namespace odyssee::net
