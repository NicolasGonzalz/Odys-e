#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/world/WorldFile.hpp"
#include "odyssee/world/WorldFingerprint.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstring>
#include <utility>

using namespace odyssee;
using namespace odyssee::world;

namespace {

/// Petit monde : 5 × 3 cases en blocs de 2 (donc 3 × 2 blocs, le dernier de chaque rangée
/// déborde de la carte), hauteur d'un point = 100 × col + row (en pas bruts au-dessus de zéro).
HeightField smallTerrain() {
    HeightField t;
    t.quadsX = 5;
    t.quadsY = 3;
    t.chunkQuads = 2;
    t.chunksX = 3;
    t.chunksY = 2;
    for (std::uint32_t cy = 0; cy < t.chunksY; ++cy) {
        for (std::uint32_t cx = 0; cx < t.chunksX; ++cx) {
            for (std::uint32_t y = 0; y <= t.chunkQuads; ++y) {
                for (std::uint32_t x = 0; x <= t.chunkQuads; ++x) {
                    const std::uint32_t col = std::min(cx * t.chunkQuads + x, t.quadsX);
                    const std::uint32_t row = std::min(cy * t.chunkQuads + y, t.quadsY);
                    t.samples.push_back(static_cast<std::uint16_t>(32768 + 100 * col + row));
                }
            }
        }
    }
    return t;
}

WorldFile smallWorld() {
    WorldFile w;
    w.terrain = smallTerrain();
    w.holes = {{1, 0}, {4, 0}, {0, 2}};
    w.boxes = {{{256, -512, 1024}, {128, 64, 256}, 16384, BoxShape::Box},
               {{0, 0, 0}, {300, 300, 512}, 0, BoxShape::Cylinder}};
    w.markers = {{MarkerKind::PlayerSpawn, {1, 2, 3}, 49152, "centre_stase"},
                 {MarkerKind::Cave, {-4, 5, -6}, 0, "grotte_03"}};
    return w;
}

DecodedWorld decode(const std::vector<std::byte>& bytes) {
    return decodeWorldFile(bytes);
}

/// Position du contenu de la section n° `index` (lue dans le répertoire).
std::size_t sectionOffset(const std::vector<std::byte>& bytes, std::size_t index) {
    const std::span<const std::byte> view(bytes);
    net::ByteReader in(view.subspan(8 + index * 20 + 4));
    return static_cast<std::size_t>(*in.readU64());
}

} // namespace

TEST_CASE("Fichier monde : écrit puis relu, il est identique") {
    const WorldFile original = smallWorld();
    const DecodedWorld decoded = decode(encodeWorldFile(original));
    REQUIRE(decoded.error == WorldFileError::None);
    REQUIRE(decoded.world.has_value());
    const WorldFile& w = *decoded.world;
    CHECK(w.terrain.samples == original.terrain.samples);
    CHECK(w.terrain.quadsX == 5);
    CHECK(w.terrain.chunksY == 2);
    CHECK(w.holes == original.holes);
    CHECK(w.boxes == original.boxes);
    CHECK(w.markers == original.markers);
}

TEST_CASE("Fichier monde : hauteur d'un point, à travers les blocs et bornée aux bords") {
    const HeightField t = smallTerrain();
    CHECK(t.raw(0, 0) == 32768);
    CHECK(t.raw(2, 1) == 32768 + 201); // frontière de deux blocs
    CHECK(t.raw(5, 3) == 32768 + 503); // coin opposé, dans un bloc qui déborde
    CHECK(t.raw(-7, 99) == t.raw(0, 3));
    CHECK(t.height(3, 2) == (302) * 4); // 1 pas brut = 1/64 m = 4/256 m
}

TEST_CASE("Fichier monde : codage des hauteurs (1/64 m, arrondi, borné)") {
    CHECK(heightFromRaw(32768) == 0);
    CHECK(heightFromRaw(32768 + 64) == 256); // 1 m
    CHECK(rawFromHeight(256) == 32768 + 64);
    CHECK(rawFromHeight(-256) == 32768 - 64);
    CHECK(rawFromHeight(5) == 32768 + 1); // 5/256 m → arrondi à 4/256 m
    CHECK(rawFromHeight(-5) == 32768 - 1);
    CHECK(rawFromHeight(1 << 30) == 65535);
}

TEST_CASE("Fichier monde : cases percées") {
    const WorldFile w = smallWorld();
    CHECK(w.isHole({1, 0}));
    CHECK(w.isHole({0, 2}));
    CHECK_FALSE(w.isHole({0, 0}));
}

TEST_CASE("Fichier monde : signature, version et troncature refusées") {
    std::vector<std::byte> bytes = encodeWorldFile(smallWorld());
    SUBCASE("signature") {
        bytes[0] = std::byte{'X'};
        CHECK(decode(bytes).error == WorldFileError::BadMagic);
    }
    SUBCASE("version") {
        bytes[4] = std::byte{2};
        CHECK(decode(bytes).error == WorldFileError::UnsupportedVersion);
    }
    SUBCASE("en-tête tronqué") {
        bytes.resize(5);
        CHECK(decode(bytes).error == WorldFileError::Truncated);
    }
    SUBCASE("sections hors du fichier") {
        bytes.resize(bytes.size() - 1);
        CHECK(decode(bytes).error == WorldFileError::BadDirectory);
    }
}

TEST_CASE("Fichier monde : relief incohérent refusé") {
    WorldFile w = smallWorld();
    SUBCASE("nombre de blocs faux") {
        w.terrain.chunksX = 2;
    }
    SUBCASE("hauteurs manquantes") {
        w.terrain.samples.pop_back();
    }
    SUBCASE("blocs vides") {
        w.terrain.chunkQuads = 0;
    }
    CHECK(decode(encodeWorldFile(w)).error == WorldFileError::BadTerrain);
}

TEST_CASE("Fichier monde : trous, blocs et marqueurs invalides refusés") {
    WorldFile w = smallWorld();
    WorldFileError expected = WorldFileError::None;
    SUBCASE("trou hors de la carte") {
        w.holes.push_back({0, 3});
        expected = WorldFileError::BadHoles;
    }
    SUBCASE("trous dans le désordre") {
        std::swap(w.holes[0], w.holes[1]);
        expected = WorldFileError::BadHoles;
    }
    SUBCASE("bloc plat") {
        w.boxes[0].halfExtents[2] = 0;
        expected = WorldFileError::BadBoxes;
    }
    SUBCASE("identifiant trop long") {
        w.markers[0].id = std::string(65, 'a');
        expected = WorldFileError::BadMarkers;
    }
    CHECK(decode(encodeWorldFile(w)).error == expected);
}

TEST_CASE("Fichier monde : une forme ou un type inconnus sont refusés") {
    std::vector<std::byte> bytes = encodeWorldFile(smallWorld());
    SUBCASE("forme de bloc") {
        const std::size_t boxes = sectionOffset(bytes, 2);
        bytes[boxes + 4 + 26] = std::byte{7}; // compte (4), puis forme du 1er bloc
        CHECK(decode(bytes).error == WorldFileError::BadBoxes);
    }
    SUBCASE("type de marqueur") {
        const std::size_t markers = sectionOffset(bytes, 3);
        bytes[markers + 4] = std::byte{9};
        CHECK(decode(bytes).error == WorldFileError::BadMarkers);
    }
}

TEST_CASE("Fichier monde : une section inconnue est ignorée, sans relief c'est une erreur") {
    std::vector<std::byte> bytes = encodeWorldFile(smallWorld());
    SUBCASE("étiquette inconnue à la place des marqueurs") {
        std::memcpy(&bytes[8 + 3 * 20], "ZZZZ", 4);
        const DecodedWorld decoded = decode(bytes);
        REQUIRE(decoded.error == WorldFileError::None);
        CHECK(decoded.world->markers.empty());
    }
    SUBCASE("relief absent") {
        std::memcpy(&bytes[8], "ZZZZ", 4);
        CHECK(decode(bytes).error == WorldFileError::MissingTerrain);
    }
    SUBCASE("relief en double") {
        std::memcpy(&bytes[8 + 20], "TERR", 4);
        CHECK(decode(bytes).error == WorldFileError::BadDirectory);
    }
}

TEST_CASE("Fichier monde : empreinte stable, différente dès qu'un octet change") {
    auto bytes = encodeWorldFile(smallWorld());
    const auto first = decodeWorldFile(bytes);
    const auto again = decodeWorldFile(bytes);
    REQUIRE(first.world);
    REQUIRE(again.world);
    CHECK(first.world->fingerprint == again.world->fingerprint);
    CHECK(first.world->fingerprint == fingerprintOf(bytes));
    CHECK(first.world->fingerprint != kNoMapFingerprint);

    bytes.back() ^= std::byte{1}; // dernier octet : un marqueur, toujours valide
    const auto changed = decodeWorldFile(bytes);
    if (changed.world) {
        CHECK(changed.world->fingerprint != first.world->fingerprint);
    }
    CHECK(fingerprintOf(bytes) != first.world->fingerprint);
}
