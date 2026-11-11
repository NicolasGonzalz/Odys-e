#include "odyssee/net/ByteBuffer.hpp"

#include <doctest/doctest.h>

#include <vector>

using odyssee::net::ByteReader;
using odyssee::net::ByteWriter;

TEST_CASE("ByteBuffer : aller-retour de tous les types") {
    ByteWriter w;
    w.writeU8(0xAB);
    w.writeU16(0xBEEF);
    w.writeU32(0xDEADBEEF);
    w.writeU64(0x0123456789ABCDEFULL);
    w.writeF32(-3.25f);
    w.writeBool(true);
    w.writeString("Odyssée");

    ByteReader r(w.data());
    CHECK(r.readU8() == 0xAB);
    CHECK(r.readU16() == 0xBEEF);
    CHECK(r.readU32() == 0xDEADBEEF);
    CHECK(r.readU64() == 0x0123456789ABCDEFULL);
    CHECK(r.readF32() == -3.25f);
    CHECK(r.readBool() == true);
    CHECK(r.readString() == "Odyssée");
    CHECK(r.remaining() == 0);
}

TEST_CASE("ByteBuffer : encodage little-endian") {
    ByteWriter w;
    w.writeU32(0x11223344);
    const auto d = w.data();
    REQUIRE(d.size() == 4);
    CHECK(std::to_integer<int>(d[0]) == 0x44);
    CHECK(std::to_integer<int>(d[3]) == 0x11);
}

TEST_CASE("ByteBuffer : les lectures hors limites échouent proprement") {
    ByteWriter w;
    w.writeU16(42);
    ByteReader r(w.data());
    CHECK_FALSE(r.readU32().has_value());
    CHECK(r.readU16() == 42);
    CHECK_FALSE(r.readU8().has_value());
}

TEST_CASE("ByteBuffer : une chaîne dont la longueur annoncée dépasse le tampon est rejetée") {
    ByteWriter w;
    w.writeU16(500); // longueur mensongère
    w.writeU8('x');
    ByteReader r(w.data());
    CHECK_FALSE(r.readString(1000).has_value());
}

TEST_CASE("ByteBuffer : un booléen invalide est rejeté") {
    ByteWriter w;
    w.writeU8(7);
    ByteReader r(w.data());
    CHECK_FALSE(r.readBool().has_value());
}

TEST_CASE("ByteBuffer : octets bruts") {
    const std::vector<std::byte> payload{std::byte{1}, std::byte{2}, std::byte{3}};
    ByteWriter w;
    w.writeBytes(payload);
    ByteReader r(w.data());
    CHECK(r.readBytes(3) == payload);
    CHECK_FALSE(r.readBytes(1).has_value()); // plus rien
}

TEST_CASE("ByteBuffer : entiers signés (complément à deux, little-endian)") {
    ByteWriter w;
    w.writeI8(-128);
    w.writeI16(-2);
    w.writeI32(-123456789);
    w.writeI32(2147483647);

    // -2 sur 16 bits : 0xFFFE, écrit poids faible d'abord.
    CHECK(w.data()[1] == std::byte{0xFE});
    CHECK(w.data()[2] == std::byte{0xFF});

    ByteReader r(w.data());
    CHECK(r.readI8() == std::int8_t{-128});
    CHECK(r.readI16() == std::int16_t{-2});
    CHECK(r.readI32() == -123456789);
    CHECK(r.readI32() == 2147483647);
    CHECK_FALSE(r.readI8().has_value());
}
