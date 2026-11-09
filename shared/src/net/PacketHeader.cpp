#include "odyssee/net/PacketHeader.hpp"

#include "odyssee/core/Config.hpp"

namespace odyssee::net {

void PacketHeader::write(ByteWriter& writer) const {
    writer.writeU16(protocol);
    writer.writeU16(sequence);
    writer.writeU16(ack);
    writer.writeU32(ackBits);
}

std::optional<PacketHeader> PacketHeader::read(ByteReader& reader) {
    const auto protocol = reader.readU16();
    const auto sequence = reader.readU16();
    const auto ack = reader.readU16();
    const auto ackBits = reader.readU32();

    if (!protocol || !sequence || !ack || !ackBits) {
        return std::nullopt;
    }
    return PacketHeader{*protocol, *sequence, *ack, *ackBits};
}

bool PacketHeader::hasCurrentProtocol() const { return protocol == kProtocolVersion; }

} // namespace odyssee::net
