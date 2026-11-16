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

void writeBody(ByteWriter& w, const PlayerDied& m) {
    w.writeU16(m.bag);
}

void writeBody(ByteWriter& w, const ChooseSurvivor& m) {
    w.writeU8(m.survivor);
}

void writeBody(ByteWriter& w, const SurvivorStatus& m) {
    w.writeU8(m.yours);
    w.writeU8(m.taken);
    w.writeU8(m.awakening ? 1 : 0);
}

void writeBody(ByteWriter& w, const JournalEntry& m) {
    w.writeU16(m.clue);
    w.writeU8(static_cast<std::uint8_t>(m.version));
    w.writeU8(m.finder);
}

void writeBody(ByteWriter& w, const ClueLocked& m) {
    w.writeU16(m.clue);
    w.writeU8(m.owner);
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

std::optional<GameEvent> readPlayerDied(ByteReader& r) {
    const auto bag = r.readU16();
    if (!bag) {
        return std::nullopt;
    }
    return PlayerDied{*bag};
}

std::optional<GameEvent> readChooseSurvivor(ByteReader& r) {
    const auto survivor = r.readU8();
    if (!survivor || *survivor == kNoSurvivor) {
        return std::nullopt;
    }
    return ChooseSurvivor{*survivor};
}

std::optional<GameEvent> readSurvivorStatus(ByteReader& r) {
    const auto yours = r.readU8();
    const auto taken = r.readU8();
    const auto awakening = r.readU8();
    if (!yours || !taken || !awakening || *awakening > 1) {
        return std::nullopt;
    }
    return SurvivorStatus{*yours, *taken, *awakening == 1};
}

std::optional<GameEvent> readJournalEntry(ByteReader& r) {
    const auto clue = r.readU16();
    const auto version = r.readU8();
    const auto finder = r.readU8();
    const bool known = version && (*version == static_cast<std::uint8_t>(ClueVersion::Full) ||
                                   *version == static_cast<std::uint8_t>(ClueVersion::Generic));
    if (!clue || !known || !finder) {
        return std::nullopt;
    }
    return JournalEntry{*clue, static_cast<ClueVersion>(*version), *finder};
}

std::optional<GameEvent> readClueLocked(ByteReader& r) {
    const auto clue = r.readU16();
    const auto owner = r.readU8();
    if (!clue || !owner) {
        return std::nullopt;
    }
    return ClueLocked{*clue, *owner};
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
    case GameEventType::PlayerDied:
        return readPlayerDied(r);
    case GameEventType::ChooseSurvivor:
        return readChooseSurvivor(r);
    case GameEventType::SurvivorStatus:
        return readSurvivorStatus(r);
    case GameEventType::JournalEntry:
        return readJournalEntry(r);
    case GameEventType::ClueLocked:
        return readClueLocked(r);
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
