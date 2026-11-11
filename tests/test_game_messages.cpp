#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/GameMessages.hpp"
#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <vector>

using namespace odyssee::net;

namespace {

std::optional<GameMessage> roundTrip(const GameMessage& message) {
    return decodeGameMessage(static_cast<std::uint8_t>(typeOf(message)),
                             encodeGameMessage(message));
}

InputCommand walkingNorth() {
    const auto buttons = static_cast<std::uint16_t>(InputButton::Sprint);
    return InputCommand{quantizeAxis(1.0f), 0, quantizeYaw(270.0f), quantizePitch(-10.0f),
                        buttons};
}

InputBatch batchOf(Tick firstTick, std::size_t count) {
    InputBatch batch{firstTick, {}};
    for (std::size_t i = 0; i < count; ++i) {
        InputCommand command = walkingNorth();
        command.moveRight = static_cast<std::int8_t>(i); // chaque input est différent
        batch.commands.push_back(command);
    }
    return batch;
}

EntityState player(EntityId id) {
    return EntityState{id, EntityKind::Player,
                       {quantizePosition(1900.0f), quantizePosition(2600.0f),
                        quantizePosition(10.3f)},
                       quantizeYaw(90.0f), 0};
}

/// Contenu brut d'un InputBatch d'un seul input, modifiable octet par octet.
std::vector<std::byte> rawBatch(Tick firstTick, const InputCommand& command) {
    return encodeGameMessage(InputBatch{firstTick, {command}});
}

} // namespace

TEST_CASE("GameMessages : aller-retour d'un lot d'inputs") {
    const InputBatch batch = batchOf(1000, 5);
    const auto decoded = roundTrip(batch);
    REQUIRE(decoded.has_value());
    CHECK(std::get<InputBatch>(*decoded) == batch);
    CHECK(std::get<InputBatch>(*decoded).tickAt(4) == 1004);
    CHECK(std::get<InputBatch>(*decoded).commands[0].isPressed(InputButton::Sprint));
    CHECK_FALSE(std::get<InputBatch>(*decoded).commands[0].isPressed(InputButton::Jump));
}

TEST_CASE("GameMessages : un lot trop long ne garde que les inputs les plus récents") {
    const InputBatch batch = batchOf(100, odyssee::kMaxInputsPerBatch + 3);
    const auto decoded = roundTrip(batch);
    REQUIRE(decoded.has_value());
    const auto& kept = std::get<InputBatch>(*decoded);
    CHECK(kept.commands.size() == odyssee::kMaxInputsPerBatch);
    CHECK(kept.firstTick == 103);
    CHECK(kept.commands.back() == batch.commands.back());
}

TEST_CASE("GameMessages : aller-retour d'un snapshot") {
    const Snapshot snapshot{4242, 1004, {player(0), player(1), player(7)}, {{-45, 12, -6}, 731}};
    const auto decoded = roundTrip(snapshot);
    REQUIRE(decoded.has_value());
    CHECK(std::get<Snapshot>(*decoded) == snapshot);

    const Snapshot empty{1, 0, {}};
    CHECK(roundTrip(empty) == GameMessage{empty});
}

TEST_CASE("GameMessages : un snapshot plein tient dans un paquet") {
    Snapshot snapshot{1, 1, {}};
    for (EntityId id = 0; id < odyssee::kMaxSnapshotEntities + 5; ++id) {
        snapshot.entities.push_back(player(id));
    }
    const auto bytes = encodeGameMessage(snapshot);
    CHECK(bytes.size() < odyssee::kMaxPacketSize - 32); // place pour l'en-tête et le cadrage

    const auto decoded = decodeGameMessage(static_cast<std::uint8_t>(GameMessageType::Snapshot),
                                           bytes);
    REQUIRE(decoded.has_value());
    CHECK(std::get<Snapshot>(*decoded).entities.size() == odyssee::kMaxSnapshotEntities);
}

TEST_CASE("GameMessages : inputs invalides refusés") {
    const auto type = static_cast<std::uint8_t>(GameMessageType::Inputs);

    SUBCASE("lot vide") {
        ByteWriter w;
        w.writeU32(10);
        w.writeU8(0);
        CHECK_FALSE(decodeGameMessage(type, w.data()).has_value());
    }
    SUBCASE("axe à -128 (jamais produit par quantizeAxis)") {
        auto bytes = rawBatch(10, walkingNorth());
        bytes[5] = std::byte{0x80}; // moveForward : après firstTick (4) et count (1)
        CHECK_FALSE(decodeGameMessage(type, bytes).has_value());
    }
    SUBCASE("inclinaison à -32768") {
        InputCommand command = walkingNorth();
        command.pitch = -32768;
        CHECK_FALSE(decodeGameMessage(type, rawBatch(10, command)).has_value());
    }
    SUBCASE("touche inconnue") {
        InputCommand command = walkingNorth();
        command.buttons = 0x8000;
        CHECK_FALSE(decodeGameMessage(type, rawBatch(10, command)).has_value());
    }
    SUBCASE("ticks qui dépasseraient 2^32") {
        InputBatch batch = batchOf(0xFFFFFFFF, 2);
        CHECK_FALSE(roundTrip(batch).has_value());
        CHECK(roundTrip(batchOf(0xFFFFFFFE, 2)).has_value());
    }
    SUBCASE("tronqué ou octets en trop") {
        auto bytes = rawBatch(10, walkingNorth());
        auto truncated = bytes;
        truncated.pop_back();
        CHECK_FALSE(decodeGameMessage(type, truncated).has_value());
        bytes.push_back(std::byte{0});
        CHECK_FALSE(decodeGameMessage(type, bytes).has_value());
    }
}

TEST_CASE("GameMessages : snapshots invalides refusés") {
    const auto type = static_cast<std::uint8_t>(GameMessageType::Snapshot);

    SUBCASE("entité en double") {
        const Snapshot twice{1, 1, {player(3), player(3)}};
        CHECK_FALSE(decodeGameMessage(type, encodeGameMessage(twice)).has_value());
    }
    SUBCASE("sorte d'entité inconnue") {
        auto bytes = encodeGameMessage(Snapshot{1, 1, {player(3)}});
        bytes[19] = std::byte{99}; // kind : après ticks (8), self (8), compteur (1) et id (2)
        CHECK_FALSE(decodeGameMessage(type, bytes).has_value());
    }
    SUBCASE("trop d'entités annoncées") {
        ByteWriter w;
        w.writeU32(1);
        w.writeU32(1);
        for (int i = 0; i < 4; ++i) {
            w.writeU16(0); // self : vitesse (3) et endurance
        }
        w.writeU8(static_cast<std::uint8_t>(odyssee::kMaxSnapshotEntities + 1));
        CHECK_FALSE(decodeGameMessage(type, w.data()).has_value());
    }
}

TEST_CASE("GameMessages : type de message inconnu refusé") {
    const auto bytes = encodeGameMessage(Snapshot{1, 1, {}});
    CHECK_FALSE(decodeGameMessage(0, bytes).has_value());
    CHECK_FALSE(decodeGameMessage(200, bytes).has_value());
}

TEST_CASE("Inputs : le tick 0 est refusé (les ticks d'input commencent à 1)") {
    const auto type = static_cast<std::uint8_t>(GameMessageType::Inputs);
    CHECK_FALSE(decodeGameMessage(type, rawBatch(0, InputCommand{})).has_value());
    CHECK(decodeGameMessage(type, rawBatch(1, InputCommand{})).has_value());
}
