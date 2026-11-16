// L'histoire dans le monde (T049) : survivants, indices lus et journal commun.
#include "GameWorld.hpp"

#include "odyssee/core/Log.hpp"

#include <utility>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"histoire"};

std::uint8_t wire(std::optional<data::SurvivorId> survivor) {
    return survivor ? survivor->value : net::kNoSurvivor;
}

net::JournalEntry toEntry(const JournalRecord& record) {
    return net::JournalEntry{record.clue.value, record.version, wire(record.finder)};
}

} // namespace

std::size_t GameWorld::useStory(data::StoryCatalog story) {
    _story = std::move(story);
    return _clues.place(_registry, *_story, *_terrain, _spawn.position);
}

std::optional<data::SurvivorId> GameWorld::survivorOf(net::PlayerId player) const {
    const PlayerControl* control = controlOf(player);
    return control ? control->survivor : std::nullopt;
}

void GameWorld::greetStory(net::PlayerId player) {
    const PlayerControl* control = controlOf(player);
    if (!control || !_story) {
        return;
    }
    _outbox[player].push_back(statusFor(*control, false));
    for (const JournalRecord& record : _journal.records()) {
        if (visibleTo(record, control->survivor)) {
            _outbox[player].push_back(toEntry(record));
        }
    }
}

void GameWorld::chooseSurvivor(net::PlayerId player, data::SurvivorId survivor) {
    PlayerControl* control = player < kMaxPlayers && _players[player]
                                 ? _registry.get<PlayerControl>(*_players[player])
                                 : nullptr;
    const bool known = _story && _story->survivor(survivor);
    if (!control || !control->saved || control->survivor || !known ||
        !_roster.assign(survivor, control->name)) {
        if (control) { // refusé : on renvoie l'état réel
            _outbox[player].push_back(statusFor(*control, false));
        }
        return;
    }
    control->survivor = survivor;
    kLog.info("{} incarne {}", control->name, _story->survivor(survivor)->name);
    sendStatusToAll();
    _outbox[player].push_back(statusFor(*control, true)); // son réveil : l'écran de la capsule
}

void GameWorld::readAimedClue(const PlayerControl& control, const Motion& motion) {
    const auto primary = static_cast<std::uint16_t>(net::InputButton::Primary);
    if (!_story || (control.pressedThisTick & primary) == 0) {
        return;
    }
    const auto aimed = _clues.target(_registry, motion.state.position, motion.state.yaw);
    const data::ClueDef* clue = aimed ? _story->clue(*aimed) : nullptr;
    if (clue) {
        recordReading(control, *clue, readClue(*clue, control.survivor, _roster));
    }
}

void GameWorld::recordReading(const PlayerControl& control, const data::ClueDef& clue,
                              const ClueReading& reading) {
    if (!reading.version) {
        _outbox[control.id].push_back(net::ClueLocked{clue.id.value, wire(reading.lockedFor)});
        return;
    }
    const JournalRecord record{clue.id, *reading.version, control.survivor};
    if (!_journal.add(record)) {
        // Déjà lu : on le remontre à celui qui le relit, sans rien changer au journal.
        _outbox[control.id].push_back(toEntry(record));
        return;
    }
    kLog.info("{} lit l'indice « {} »", control.name, clue.key);
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        const PlayerControl* other = controlOf(id);
        if (other && (id == control.id || visibleTo(record, other->survivor))) {
            _outbox[id].push_back(toEntry(record));
        }
    }
}

void GameWorld::sendStatusToAll() {
    for (net::PlayerId id = 0; id < kMaxPlayers; ++id) {
        if (const PlayerControl* control = controlOf(id)) {
            _outbox[id].push_back(statusFor(*control, false));
        }
    }
}

net::SurvivorStatus GameWorld::statusFor(const PlayerControl& control, bool awakening) const {
    return net::SurvivorStatus{wire(control.survivor), _roster.takenMask(), awakening};
}

bool GameWorld::visibleTo(const JournalRecord& record,
                          std::optional<data::SurvivorId> reader) const {
    const data::ClueDef* clue = _story ? _story->clue(record.clue) : nullptr;
    const bool ownerOnly = clue && clue->personal() && record.version == net::ClueVersion::Full;
    return !ownerOnly || reader == clue->owner;
}

bool GameWorld::clueReadBy(data::ClueId clue, std::optional<data::SurvivorId> reader) const {
    for (const JournalRecord& record : _journal.records()) {
        if (record.clue == clue && visibleTo(record, reader)) {
            return true;
        }
    }
    return false;
}

void GameWorld::saveStory(WorldSave& save) const {
    if (!_story) {
        return;
    }
    for (const data::SurvivorDef& survivor : _story->survivors()) {
        if (_roster.taken(survivor.id)) {
            save.survivors.push_back({survivor.key, std::string(_roster.ownerOf(survivor.id))});
        }
    }
    for (const JournalRecord& record : _journal.records()) {
        const data::ClueDef* clue = _story->clue(record.clue);
        const data::SurvivorDef* finder =
            record.finder ? _story->survivor(*record.finder) : nullptr;
        if (clue) {
            save.journal.push_back({clue->key, static_cast<std::uint8_t>(record.version),
                                    finder ? finder->key : std::string()});
        }
    }
}

void GameWorld::loadStory(const WorldSave& save) {
    if (!_story) {
        return;
    }
    for (const SavedSurvivor& saved : save.survivors) {
        if (const data::SurvivorDef* survivor = _story->findSurvivor(saved.survivor)) {
            (void)_roster.assign(survivor->id, saved.player);
        }
    }
    for (const SavedJournalRecord& saved : save.journal) {
        const data::ClueDef* clue = _story->findClue(saved.clue); // indice retiré : oublié
        const data::SurvivorDef* finder = _story->findSurvivor(saved.finder);
        const bool knownVersion = saved.version == 1 || saved.version == 2;
        if (clue && knownVersion) {
            (void)_journal.add({clue->id, static_cast<net::ClueVersion>(saved.version),
                                finder ? std::optional(finder->id) : std::nullopt});
        }
    }
}

} // namespace odyssee::server
