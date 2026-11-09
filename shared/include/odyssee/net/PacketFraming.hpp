#pragma once

#include "odyssee/net/PacketHeader.hpp"
#include "odyssee/net/PacketKind.hpp"

#include <cstddef>
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

/// Assemble un paquet complet prêt à être envoyé.
[[nodiscard]] std::vector<std::byte> buildPacket(const PacketHeader& header, PacketKind kind,
                                                 std::span<const std::byte> body);

} // namespace odyssee::net
