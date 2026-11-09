#include "odyssee/net/ReliableCodec.hpp"

#include <algorithm>

namespace odyssee::net::reliable_codec {

namespace {

std::optional<ReliableMessage> readOne(ByteReader& reader) {
    const auto id = reader.readU16();
    const auto size = reader.readU16();
    if (!id || !size) {
        return std::nullopt;
    }

    auto payload = reader.readBytes(*size);
    if (!payload) {
        return std::nullopt; // taille annoncée plus grande que ce qui reste
    }
    return ReliableMessage{*id, std::move(*payload)};
}

} // namespace

void write(ByteWriter& writer, std::span<const ReliableMessage> messages) {
    const auto count = std::min(messages.size(), kMaxMessagesPerPacket);
    writer.writeU8(static_cast<std::uint8_t>(count));

    for (const ReliableMessage& message : messages.first(count)) {
        writer.writeU16(message.id);
        writer.writeU16(static_cast<std::uint16_t>(message.payload.size()));
        writer.writeBytes(message.payload);
    }
}

std::optional<std::vector<ReliableMessage>> read(ByteReader& reader) {
    const auto count = reader.readU8();
    if (!count || *count > kMaxMessagesPerPacket) {
        return std::nullopt;
    }

    std::vector<ReliableMessage> messages;
    messages.reserve(*count);
    for (std::size_t i = 0; i < *count; ++i) {
        auto message = readOne(reader);
        if (!message) {
            return std::nullopt;
        }
        messages.push_back(std::move(*message));
    }
    return messages;
}

} // namespace odyssee::net::reliable_codec
