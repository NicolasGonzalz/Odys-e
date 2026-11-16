#pragma once

#include "PlayerComponents.hpp"
#include "Vitals.hpp"
#include "death/DroppedBags.hpp"

#include "odyssee/net/GameEvents.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace odyssee::server {

/// Ce qu'une sauvegarde garde du monde (T053), en données simples : le monde le remplit
/// (GameWorld::save) et le relit (GameWorld::load) ; SaveStore l'écrit dans la base SQLite.

/// Un joueur, connecté ou non, retrouvé par son nom à sa prochaine connexion.
struct SavedPlayer {
    std::string name;
    std::array<std::int32_t, 3> position{}; ///< 1/256 m
    std::uint16_t yaw = 0;
    Vitals vitals;
    DroppedSlots inventory{}; ///< les 25 cases, dans l'ordre
    RestPoint rest;
    std::vector<net::MapChunk> pendingMap; ///< souvenirs pas encore reportés (T074)

    friend bool operator==(const SavedPlayer&, const SavedPlayer&) = default;
};

/// L'état d'une ressource récoltable, par son rang de création (le placement, lui, se refait
/// à l'identique au démarrage : même carte, mêmes données).
struct SavedResource {
    std::uint8_t type = 0; ///< rang du type dans data/resources.txt, pour vérifier à la reprise
    std::uint16_t hitsLeft = 0;
    std::uint32_t regrowTicksLeft = 0;

    friend bool operator==(const SavedResource&, const SavedResource&) = default;
};

/// Un sac au sol (T052).
struct SavedBag {
    std::array<std::int32_t, 3> position{};
    BagContents contents;

    friend bool operator==(const SavedBag&, const SavedBag&) = default;
};

/// Une lecture du journal (T049), par clés stables (data/story.txt).
struct SavedJournalRecord {
    std::string clue;
    std::uint8_t version = 1; ///< net::ClueVersion
    std::string finder;       ///< clé du survivant qui l'a lu (vide : sans survivant)

    friend bool operator==(const SavedJournalRecord&, const SavedJournalRecord&) = default;
};

/// Qui incarne quel survivant (T049) : clé du survivant, nom du joueur.
struct SavedSurvivor {
    std::string survivor;
    std::string player;

    friend bool operator==(const SavedSurvivor&, const SavedSurvivor&) = default;
};

struct WorldSave {
    /// Empreinte de la carte au moment de la sauvegarde (world::fingerprintOf ; 0 sans carte).
    std::uint64_t worldFingerprint = 0;
    std::vector<SavedPlayer> players;
    std::vector<SavedResource> resources;
    std::vector<SavedBag> bags;
    std::vector<SavedSurvivor> survivors;    ///< T049
    std::vector<SavedJournalRecord> journal; ///< T049, dans l'ordre de lecture
    std::vector<net::MapChunk> map;          ///< carte commune (T074)

    friend bool operator==(const WorldSave&, const WorldSave&) = default;
};

} // namespace odyssee::server
