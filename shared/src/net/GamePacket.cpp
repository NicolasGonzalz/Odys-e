#include "odyssee/net/GamePacket.hpp"

#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/PacketFraming.hpp"
#include "odyssee/net/PacketKind.hpp"
#include "odyssee/net/ReliableCodec.hpp"

#include <algorithm>

namespace odyssee::net::game_packet {

namespace {

constexpr std::size_t kKindSize = 1;

void writeUnreliable(ByteWriter& writer, std::span<const UnreliableMessage> messages) {
    const auto count = std::min(messages.size(), kMaxUnreliablePerPacket);
    writer.writeU8(static_cast<std::uint8_t>(count));
    for (const UnreliableMessage& message : messages.first(count)) {
        writer.writeU8(message.type);
        writer.writeU16(static_cast<std::uint16_t>(message.data.size()));
        writer.writeBytes(message.data);
    }
}

std::optional<UnreliableMessage> readOneUnreliable(ByteReader& reader) {
    const auto type = reader.readU8();
    const auto size = reader.readU16();
    if (!type || !size) {
        return std::nullopt;
    }
    auto data = reader.readBytes(*size);
    if (!data) {
        return std::nullopt; // taille annoncée plus grande que ce qui reste
    }
    return UnreliableMessage{*type, std::move(*data)};
}

std::optional<std::vector<UnreliableMessage>> readUnreliable(ByteReader& reader) {
    const auto count = reader.readU8();
    if (!count || *count > kMaxUnreliablePerPacket) {
        return std::nullopt;
    }
    std::vector<UnreliableMessage> messages;
    messages.reserve(*count);
    for (std::size_t i = 0; i < *count; ++i) {
        auto message = readOneUnreliable(reader);
        if (!message) {
            return std::nullopt;
        }
        messages.push_back(std::move(*message));
    }
    return messages;
}

} // namespace

std::size_t packetSize(const GamePacketBody& body) {
    std::size_t size = PacketHeader::kSize + kKindSize + reliable_codec::kListOverhead +
                       kUnreliableListOverhead;
    for (const ReliableMessage& message : body.reliable) {
        size += reliable_codec::encodedSize(message);
    }
    for (const UnreliableMessage& message : body.unreliable) {
        size += encodedSize(message);
    }
    return size;
}

std::vector<std::byte> build(const PacketHeader& header, const GamePacketBody& body) {
    ByteWriter writer;
    reliable_codec::write(writer, body.reliable);
    writeUnreliable(writer, body.unreliable);
    return buildPacket(header, PacketKind::Game, writer.data());
}

std::optional<GamePacketBody> parse(std::span<const std::byte> body) {
    ByteReader reader(body);
    auto reliable = reliable_codec::read(reader);
    if (!reliable) {
        return std::nullopt;
    }
    auto unreliable = readUnreliable(reader);
    if (!unreliable || reader.remaining() != 0) {
        return std::nullopt; // tronqué, ou octets en trop
    }
    return GamePacketBody{std::move(*reliable), std::move(*unreliable)};
}

} // namespace odyssee::net::game_packet
