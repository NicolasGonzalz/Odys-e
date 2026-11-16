#pragma once

#include "story/SurvivorRoster.hpp"

#include "odyssee/data/StoryDef.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <optional>

namespace odyssee::server {

/// Ce que donne la lecture d'un indice (T049, docs/HISTOIRE.md § 4).
struct ClueReading {
    /// Version qui entre au journal ; vide : l'indice ne se lit pas (voir lockedFor).
    std::optional<net::ClueVersion> version;
    /// Indice personnel d'un survivant joué par quelqu'un d'autre : lui seul saurait le lire.
    std::optional<data::SurvivorId> lockedFor;
    /// La version complète d'un indice personnel n'est montrée qu'à son survivant.
    bool ownerOnly = false;
};

/// Lecture de `clue` par un joueur qui incarne `reader` (vide : il n'a pas encore choisi).
/// - Commun : texte complet, pour tous.
/// - Personnel lu par son survivant : texte complet et souvenir, pour lui seul.
/// - Personnel d'un survivant joué par personne : version générique, pour tous.
/// - Personnel d'un survivant joué par un autre : illisible ici.
[[nodiscard]] ClueReading readClue(const data::ClueDef& clue,
                                   std::optional<data::SurvivorId> reader,
                                   const SurvivorRoster& roster);

} // namespace odyssee::server
