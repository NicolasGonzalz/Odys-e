#include "StoryState.hpp"

#include <algorithm>
#include <set>
#include <variant>

namespace odyssee::debugclient {

bool StoryState::onEvent(const net::GameEvent& event, double now) {
    if (const auto* status = std::get_if<net::SurvivorStatus>(&event)) {
        onStatus(*status, now);
        return true;
    }
    if (const auto* entry = std::get_if<net::JournalEntry>(&event)) {
        onEntry(*entry, now);
        return true;
    }
    if (const auto* locked = std::get_if<net::ClueLocked>(&event)) {
        _locked = *locked;
        _noticeUntil = now + kReadingSeconds / 2;
        return true;
    }
    return false;
}

void StoryState::onStatus(const net::SurvivorStatus& status, double now) {
    if (!_statusKnown) {
        _greetedAt = now; // le journal déjà écrit arrive juste après, dans le même lot
    }
    _statusKnown = true;
    _status = status;
    if (status.awakening) {
        _introUntil = now + kIntroSeconds;
    }
}

void StoryState::onEntry(const net::JournalEntry& entry, double now) {
    const bool known = std::any_of(_journal.begin(), _journal.end(), [&](const auto& e) {
        return e.clue == entry.clue && e.version == entry.version;
    });
    if (!known) {
        _journal.push_back(entry);
    }
    // À la connexion, le serveur renvoie tout le journal : rien à montrer. Ensuite, chaque lecture
    // (neuve ou relue) s'affiche.
    if (now > _greetedAt) {
        _reading = entry;
        _readingUntil = now + kReadingSeconds;
    }
}

std::optional<data::SurvivorId> StoryState::survivor() const {
    if (!_statusKnown || _status.yours == net::kNoSurvivor) {
        return std::nullopt;
    }
    return data::SurvivorId{_status.yours};
}

bool StoryState::isTaken(data::SurvivorId survivor) const {
    return survivor.value < 8 && (_status.taken & (1U << survivor.value)) != 0;
}

std::optional<net::JournalEntry> StoryState::reading(double now) const {
    return now < _readingUntil ? _reading : std::nullopt;
}

std::optional<std::string> StoryState::notice(double now) const {
    if (now >= _noticeUntil || !_locked) {
        return std::nullopt;
    }
    return std::string("Ce document parle à quelqu'un d'autre.");
}

std::string StoryState::lockedOwnerName(const data::StoryCatalog& story) const {
    const data::SurvivorDef* owner = _locked ? story.survivor({_locked->owner}) : nullptr;
    return owner ? owner->name : std::string();
}

std::vector<std::uint8_t> StoryState::understoodSteps(const data::StoryCatalog& story) const {
    std::set<std::uint8_t> steps;
    for (const net::JournalEntry& entry : _journal) {
        const data::ClueDef* clue = story.clue({entry.clue});
        if (clue && clue->reveals) {
            steps.insert(clue->step);
        }
    }
    return {steps.begin(), steps.end()};
}

std::vector<std::string> StoryState::memories(const data::StoryCatalog& story) const {
    std::vector<std::string> found;
    for (const net::JournalEntry& entry : _journal) {
        const data::ClueDef* clue = story.clue({entry.clue});
        if (clue && clue->personal() && entry.version == net::ClueVersion::Full &&
            clue->owner == survivor()) {
            found.push_back(clue->memory);
        }
    }
    return found;
}

} // namespace odyssee::debugclient
