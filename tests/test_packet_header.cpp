#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/PacketHeader.hpp"

#include <doctest/doctest.h>

using odyssee::net::ByteReader;
using odyssee::net::ByteWriter;
using odyssee::net::PacketHeader;

TEST_CASE("PacketHeader : aller-retour écriture/lecture sur 10 octets") {
    const PacketHeader sent{odyssee::kProtocolVersion, 1234, 1230, 0b1011};

    ByteWriter writer;
    sent.write(writer);
    REQUIRE(writer.size() == PacketHeader::kSize);

    ByteReader reader(writer.data());
    const auto received = PacketHeader::read(reader);
    REQUIRE(received.has_value());
    CHECK(received->protocol == sent.protocol);
    CHECK(received->sequence == 1234);
    CHECK(received->ack == 1230);
    CHECK(received->ackBits == 0b1011);
    CHECK(received->hasCurrentProtocol());
}

TEST_CASE("PacketHeader : un paquet trop court est refusé") {
    ByteWriter writer;
    PacketHeader{}.write(writer);
    const auto truncated = writer.data().first(PacketHeader::kSize - 1);

    ByteReader reader(truncated);
    CHECK_FALSE(PacketHeader::read(reader).has_value());
}

TEST_CASE("PacketHeader : une autre version du protocole est détectée") {
    const PacketHeader foreign{static_cast<std::uint16_t>(odyssee::kProtocolVersion + 1), 0, 0, 0};
    CHECK_FALSE(foreign.hasCurrentProtocol());
}
