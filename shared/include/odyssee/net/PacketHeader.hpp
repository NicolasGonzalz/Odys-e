#pragma once

#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/Sequence.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace odyssee::net {

/// En-tête placé au début de chaque paquet (voir docs/NETWORK.md) :
///
///   protocol (u16) | sequence (u16) | ack (u16) | ackBits (u32)   = 10 octets
struct PacketHeader {
    std::uint16_t protocol = 0;
    Sequence sequence = 0;    ///< numéro de ce paquet
    Sequence ack = 0;         ///< dernier paquet reçu de l'autre côté
    std::uint32_t ackBits = 0; ///< bit i = paquet (ack - 1 - i) reçu

    static constexpr std::size_t kSize = 10;

    /// Ajoute l'en-tête à la fin de `writer`.
    void write(ByteWriter& writer) const;

    /// Lit un en-tête. Renvoie std::nullopt si le paquet est trop court.
    [[nodiscard]] static std::optional<PacketHeader> read(ByteReader& reader);

    /// Vrai si le paquet vient d'un client ou d'un serveur de la même version que nous.
    [[nodiscard]] bool hasCurrentProtocol() const;
};

} // namespace odyssee::net
