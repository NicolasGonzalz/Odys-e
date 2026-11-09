#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace odyssee::net {

/// Numéro d'un message fiable (distinct du numéro de paquet). Reboucle comme une Sequence.
using MessageId = std::uint16_t;

/// Un message qui doit arriver, dans l'ordre : inventaire, chat, événements de jeu…
struct ReliableMessage {
    MessageId id = 0;
    std::vector<std::byte> payload;

    friend bool operator==(const ReliableMessage&, const ReliableMessage&) = default;
};

} // namespace odyssee::net
