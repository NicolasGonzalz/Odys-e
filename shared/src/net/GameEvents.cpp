#include "odyssee/net/GameEvents.hpp"

#include "odyssee/net/ByteBuffer.hpp"

namespace odyssee::net {

namespace {

void writeBody(ByteWriter& w, const InventoryContents& m) {
    for (const InventorySlot& slot : m.slots) {
        w.writeU16(slot.item);
        w.writeU16(slot.count);
    }
}

void writeBody(ByteWriter& w, const MoveItem& m) {
    w.writeU8(m.from);
    w.writeU8(m.to);
}

void writeBody(ByteWriter& w, const CraftRequest& m) {
    w.writeU16(m.recipe);
}

void writeBody(ByteWriter& w, const CraftStatus& m) {
    w.writeU16(m.recipe);
    w.writeU8(static_cast<std::uint8_t>(m.outcome));
    w.writeU16(m.ticks);
}

/// Le type d'un événement : son rang dans le variant + 1 (même ordre que GameEventType).
GameEventType typeOf(const GameEvent& message) {
    return static_cast<GameEventType>(message.index() + 1);
}

std::optional<GameEvent> readInventory(ByteReader& r) {
    InventoryContents contents;
    for (InventorySlot& slot : contents.slots) {
        const auto item = r.readU16();
        const auto count = r.readU16();
        if (!item || !count || (*count == 0 && *item != 0)) {
            return std::nullopt; // case vide : objet à 0 aussi (une seule écriture possible)
        }
        slot = {*item, *count};
    }
    return contents;
}

std::optional<GameEvent> readMove(ByteReader& r) {
    const auto from = r.readU8();
    const auto to = r.readU8();
    if (!from || !to || *from >= kInventorySlots || *to >= kInventorySlots) {
        return std::nullopt;
    }
    return MoveItem{*from, *to};
}

std::optional<GameEvent> readCraftRequest(ByteReader& r) {
    const auto recipe = r.readU16();
    if (!recipe) {
        return std::nullopt;
    }
    return CraftRequest{*recipe};
}

std::optional<GameEvent> readCraftStatus(ByteReader& r) {
    const auto recipe = r.readU16();
    const auto outcome = r.readU8();
    const auto ticks = r.readU16();
    const bool known = outcome && *outcome >= 1 && *outcome <= 6;
    if (!recipe || !known || !ticks) {
        return std::nullopt;
    }
    return CraftStatus{*recipe, static_cast<CraftOutcome>(*outcome), *ticks};
}

std::optional<GameEvent> readBody(GameEventType type, ByteReader& r) {
    switch (type) {
    case GameEventType::InventoryContents:
        return readInventory(r);
    case GameEventType::MoveItem:
        return readMove(r);
    case GameEventType::CraftRequest:
        return readCraftRequest(r);
    case GameEventType::CraftStatus:
        return readCraftStatus(r);
    }
    return std::nullopt;
}

} // namespace

std::vector<std::byte> encodeGameEvent(const GameEvent& message) {
    ByteWriter w;
    w.writeU8(static_cast<std::uint8_t>(typeOf(message)));
    std::visit([&w](const auto& body) { writeBody(w, body); }, message);
    return {w.data().begin(), w.data().end()};
}

std::optional<GameEvent> decodeGameEvent(std::span<const std::byte> bytes) {
    ByteReader r(bytes);
    const auto type = r.readU8();
    if (!type) {
        return std::nullopt;
    }
    auto message = readBody(static_cast<GameEventType>(*type), r);
    if (!message || r.remaining() != 0) {
        return std::nullopt; // inconnu, invalide, ou octets en trop
    }
    return message;
}

} // namespace odyssee::net
