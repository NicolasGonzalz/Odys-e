#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/ReliableCodec.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee::net;

namespace {

ReliableMessage message(MessageId id, std::initializer_list<int> bytes) {
    ReliableMessage m{id, {}};
    for (const int b : bytes) {
        m.payload.push_back(static_cast<std::byte>(b));
    }
    return m;
}

} // namespace

TEST_CASE("ReliableCodec : aller-retour d'une liste de messages") {
    const std::vector<ReliableMessage> sent{message(3, {1, 2}), message(4, {}), message(5, {9})};

    ByteWriter writer;
    reliable_codec::write(writer, sent);
    CHECK(writer.size() == 1 + (4 + 2) + (4 + 0) + (4 + 1));

    ByteReader reader(writer.data());
    CHECK(reliable_codec::read(reader) == sent);
}

TEST_CASE("ReliableCodec : une liste vide est valide") {
    ByteWriter writer;
    reliable_codec::write(writer, {});
    ByteReader reader(writer.data());
    const auto read = reliable_codec::read(reader);
    REQUIRE(read.has_value());
    CHECK(read->empty());
}

TEST_CASE("ReliableCodec : données tronquées ou mensongères refusées") {
    ByteWriter writer;
    writer.writeU8(1);   // un message annoncé…
    writer.writeU16(0);  // id
    writer.writeU16(50); // …de 50 octets, mais il n'y en a aucun
    ByteReader reader(writer.data());
    CHECK_FALSE(reliable_codec::read(reader).has_value());
}

TEST_CASE("ReliableCodec : trop de messages annoncés refusé") {
    ByteWriter writer;
    writer.writeU8(static_cast<std::uint8_t>(reliable_codec::kMaxMessagesPerPacket + 1));
    ByteReader reader(writer.data());
    CHECK_FALSE(reliable_codec::read(reader).has_value());
}
