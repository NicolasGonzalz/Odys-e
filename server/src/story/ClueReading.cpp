#include "story/ClueReading.hpp"

namespace odyssee::server {

ClueReading readClue(const data::ClueDef& clue, std::optional<data::SurvivorId> reader,
                     const SurvivorRoster& roster) {
    if (!clue.owner) {
        return {net::ClueVersion::Full, std::nullopt, false};
    }
    if (reader == clue.owner) {
        return {net::ClueVersion::Full, std::nullopt, true};
    }
    if (!roster.taken(*clue.owner)) {
        return {net::ClueVersion::Generic, std::nullopt, false};
    }
    return {std::nullopt, clue.owner, false};
}

} // namespace odyssee::server
