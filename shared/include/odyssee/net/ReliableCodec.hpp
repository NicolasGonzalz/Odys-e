#pragma once

#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/ReliableMessage.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace odyssee::net {

/// Format des messages fiables dans un paquet, juste après l'en-tête :
///
///   count (u8) | id (u16) | size (u16) | payload … (répété `count` fois)
namespace reliable_codec {

inline constexpr std::size_t kListOverhead = 1;      ///< le compteur
inline constexpr std::size_t kMessageOverhead = 4;   ///< id + size
inline constexpr std::size_t kMaxMessagesPerPacket = 64;

/// Place occupée par un message une fois encodé.
[[nodiscard]] constexpr std::size_t encodedSize(const ReliableMessage& message) {
    return kMessageOverhead + message.payload.size();
}

/// Écrit la liste (au plus kMaxMessagesPerPacket messages).
void write(ByteWriter& writer, std::span<const ReliableMessage> messages);

/// Lit une liste. Renvoie std::nullopt si les données sont tronquées ou incohérentes.
[[nodiscard]] std::optional<std::vector<ReliableMessage>> read(ByteReader& reader);

} // namespace reliable_codec

} // namespace odyssee::net
