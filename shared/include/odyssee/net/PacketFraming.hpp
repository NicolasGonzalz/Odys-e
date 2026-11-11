#pragma once

#include "odyssee/net/PacketHeader.hpp"
#include "odyssee/net/PacketKind.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace odyssee::net {

/// Découpage d'un paquet : en-tête (10 octets) | type (1 octet) | contenu.
///
///   PacketHeader | PacketKind | body…
struct IncomingPacket {
    PacketHeader header;
    PacketKind kind;
    std::span<const std::byte> body; ///< vue sur les octets reçus (pas de copie)
};

/// Lit l'en-tête et le type. Renvoie std::nullopt si le paquet est trop court ou de type inconnu.
/// La version du protocole n'est **pas** vérifiée ici (voir PacketHeader::hasCurrentProtocol).
[[nodiscard]] std::optional<IncomingPacket> parsePacket(std::span<const std::byte> bytes);

/// Jeton de session annoncé par un paquet d'après-connexion (Game, Disconnect, KeepAlive) : les
/// 8 premiers octets du contenu. Lu sans rien décoder d'autre, pour écarter un paquet forgé
/// avant tout autre traitement. std::nullopt pour les autres types, ou si le paquet est trop court.
[[nodiscard]] std::optional<std::uint64_t> sessionTokenOf(const IncomingPacket& packet);

/// Assemble un paquet complet prêt à être envoyé.
[[nodiscard]] std::vector<std::byte> buildPacket(const PacketHeader& header, PacketKind kind,
                                                 std::span<const std::byte> body);

} // namespace odyssee::net
