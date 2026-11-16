#pragma once

#include "PlayerComponents.hpp"
#include "Vitals.hpp"
#include "death/DroppedBags.hpp"

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

struct WorldSave {
    /// Empreinte de la carte au moment de la sauvegarde (world::fingerprintOf ; 0 sans carte).
    std::uint64_t worldFingerprint = 0;
    std::vector<SavedPlayer> players;
    std::vector<SavedResource> resources;
    std::vector<SavedBag> bags;

    friend bool operator==(const WorldSave&, const WorldSave&) = default;
};

} // namespace odyssee::server
