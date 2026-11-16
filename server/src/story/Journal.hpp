#pragma once

#include "odyssee/data/StoryDef.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <optional>
#include <span>
#include <vector>

namespace odyssee::server {

/// Un indice lu, dans le journal du monde.
struct JournalRecord {
    data::ClueId clue;
    net::ClueVersion version = net::ClueVersion::Full;
    std::optional<data::SurvivorId> finder; ///< qui l'a lu (vide : un joueur sans survivant)

    friend bool operator==(const JournalRecord&, const JournalRecord&) = default;
};

/// Le journal commun du monde (T049) : les indices lus par les joueurs, dans l'ordre de lecture.
/// Un même indice peut y être deux fois : en version générique, puis complète quand son survivant
/// le lit lui-même.
class Journal {
  public:
    /// Ajoute la lecture ; faux si cet indice est déjà là dans cette version (rien ne change).
    bool add(const JournalRecord& record);
    [[nodiscard]] bool has(data::ClueId clue, net::ClueVersion version) const;
    [[nodiscard]] std::span<const JournalRecord> records() const { return _records; }

  private:
    std::vector<JournalRecord> _records;
};

} // namespace odyssee::server
