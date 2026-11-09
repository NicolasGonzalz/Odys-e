#include "odyssee/net/PacketFraming.hpp"

#include "odyssee/net/ByteBuffer.hpp"

namespace odyssee::net {

namespace {

constexpr std::size_t kKindSize = 1;

} // namespace

std::optional<IncomingPacket> parsePacket(std::span<const std::byte> bytes) {
    ByteReader reader(bytes);

    const auto header = PacketHeader::read(reader);
    const auto kind = reader.readU8();
    if (!header || !kind || !isKnownPacketKind(*kind)) {
        return std::nullopt;
    }

    const auto body = bytes.subspan(PacketHeader::kSize + kKindSize);
    return IncomingPacket{*header, static_cast<PacketKind>(*kind), body};
}

std::vector<std::byte> buildPacket(const PacketHeader& header, PacketKind kind,
                                   std::span<const std::byte> body) {
    ByteWriter writer;
    header.write(writer);
    writer.writeU8(static_cast<std::uint8_t>(kind));
    writer.writeBytes(body);
    return {writer.data().begin(), writer.data().end()};
}

} // namespace odyssee::net
