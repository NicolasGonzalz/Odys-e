#include "odyssee/net/GameMessages.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"
#include "odyssee/net/Quantize.hpp"

#include <algorithm>
#include <limits>

namespace odyssee::net {

namespace {

// --- Écriture ---------------------------------------------------------------------------------

void writeCommand(ByteWriter& w, const InputCommand& c) {
    w.writeI8(c.moveForward);
    w.writeI8(c.moveRight);
    w.writeU16(c.yaw);
    w.writeI16(c.pitch);
    w.writeU16(c.buttons);
}

/// Au-delà de kMaxInputsPerBatch, seuls les inputs les plus récents partent.
void writeBody(ByteWriter& w, const InputBatch& batch) {
    const std::size_t count = std::min(batch.commands.size(), kMaxInputsPerBatch);
    const std::size_t skipped = batch.commands.size() - count;
    w.writeU32(batch.tickAt(skipped));
    w.writeU8(static_cast<std::uint8_t>(count));
    for (std::size_t i = skipped; i < batch.commands.size(); ++i) {
        writeCommand(w, batch.commands[i]);
    }
}

void writeEntity(ByteWriter& w, const EntityState& e) {
    w.writeU16(e.id);
    w.writeU8(static_cast<std::uint8_t>(e.kind));
    for (const std::int32_t coordinate : e.position) {
        w.writeI32(coordinate);
    }
    w.writeU16(e.yaw);
    w.writeU8(e.flags);
}

/// Au-delà de kMaxSnapshotEntities, les entités en trop ne partent pas (la zone d'intérêt doit
/// choisir avant : voir docs/NETWORK.md).
void writeBody(ByteWriter& w, const Snapshot& snapshot) {
    const std::size_t count = std::min(snapshot.entities.size(), kMaxSnapshotEntities);
    w.writeU32(snapshot.serverTick);
    w.writeU32(snapshot.lastProcessedInput);
    w.writeU8(static_cast<std::uint8_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        writeEntity(w, snapshot.entities[i]);
    }
}

// --- Lecture ----------------------------------------------------------------------------------

/// -128 n'est jamais produit par quantizeAxis : un input qui le contient est forgé.
std::optional<std::int8_t> readAxis(ByteReader& r) {
    const auto axis = r.readI8();
    if (!axis || *axis < -kMaxAxisUnits) {
        return std::nullopt;
    }
    return axis;
}

std::optional<InputCommand> readCommand(ByteReader& r) {
    const auto forward = readAxis(r);
    const auto right = readAxis(r);
    const auto yaw = r.readU16();
    const auto pitch = r.readI16();
    const auto buttons = r.readU16();
    const bool valid = forward && right && yaw && pitch && *pitch >= -kMaxPitchUnits &&
                       buttons && (*buttons & ~kKnownInputButtons) == 0;
    if (!valid) {
        return std::nullopt;
    }
    return InputCommand{*forward, *right, *yaw, *pitch, *buttons};
}

std::optional<GameMessage> readInputs(ByteReader& r) {
    const auto firstTick = r.readU32();
    const auto count = r.readU8();
    if (!firstTick || !count || *count == 0 || *count > kMaxInputsPerBatch ||
        *firstTick > std::numeric_limits<Tick>::max() - static_cast<Tick>(*count - 1)) {
        return std::nullopt; // vide, trop long, ou ticks qui dépasseraient 2^32
    }
    InputBatch batch{*firstTick, {}};
    for (std::uint8_t i = 0; i < *count; ++i) {
        const auto command = readCommand(r);
        if (!command) {
            return std::nullopt;
        }
        batch.commands.push_back(*command);
    }
    return batch;
}

std::optional<EntityState> readEntity(ByteReader& r) {
    const auto id = r.readU16();
    const auto kind = r.readU8();
    const auto x = r.readI32();
    const auto y = r.readI32();
    const auto z = r.readI32();
    const auto yaw = r.readU16();
    const auto flags = r.readU8();
    const bool knownKind = kind && *kind == static_cast<std::uint8_t>(EntityKind::Player);
    if (!id || !knownKind || !x || !y || !z || !yaw || !flags) {
        return std::nullopt;
    }
    return EntityState{*id, static_cast<EntityKind>(*kind), {*x, *y, *z}, *yaw, *flags};
}

bool containsId(const std::vector<EntityState>& entities, EntityId id) {
    return std::any_of(entities.begin(), entities.end(),
                       [id](const EntityState& e) { return e.id == id; });
}

std::optional<GameMessage> readSnapshot(ByteReader& r) {
    const auto serverTick = r.readU32();
    const auto lastInput = r.readU32();
    const auto count = r.readU8();
    if (!serverTick || !lastInput || !count || *count > kMaxSnapshotEntities) {
        return std::nullopt;
    }
    Snapshot snapshot{*serverTick, *lastInput, {}};
    for (std::uint8_t i = 0; i < *count; ++i) {
        const auto entity = readEntity(r);
        if (!entity || containsId(snapshot.entities, entity->id)) {
            return std::nullopt; // entité invalide, ou en double
        }
        snapshot.entities.push_back(*entity);
    }
    return snapshot;
}

std::optional<GameMessage> readMessage(std::uint8_t type, ByteReader& r) {
    switch (static_cast<GameMessageType>(type)) {
    case GameMessageType::Inputs:
        return readInputs(r);
    case GameMessageType::Snapshot:
        return readSnapshot(r);
    }
    return std::nullopt; // type inconnu
}

} // namespace

GameMessageType typeOf(const GameMessage& message) {
    // L'ordre des types du variant suit les valeurs de GameMessageType (Inputs = 1, …).
    return static_cast<GameMessageType>(message.index() + 1);
}

std::vector<std::byte> encodeGameMessage(const GameMessage& message) {
    ByteWriter writer;
    std::visit([&writer](const auto& m) { writeBody(writer, m); }, message);
    return {writer.data().begin(), writer.data().end()};
}

std::optional<GameMessage> decodeGameMessage(std::uint8_t type, std::span<const std::byte> body) {
    ByteReader reader(body);
    auto message = readMessage(type, reader);
    if (!message || reader.remaining() != 0) {
        return std::nullopt; // contenu invalide, ou octets en trop
    }
    return message;
}

} // namespace odyssee::net
