#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace odyssee::data {

/// L'histoire du jeu (T049, docs/HISTOIRE.md), lue dans data/story.txt : les survivants, l'écran
/// du réveil, les étapes du fil des révélations et les indices. Le texte reste dans les données,
/// côté client comme côté serveur : sur le réseau, seuls circulent des rangs.

/// Un survivant : son rang dans data/story.txt (0 à kMaxSurvivors - 1).
struct SurvivorId {
    std::uint8_t value = 0;

    friend auto operator<=>(const SurvivorId&, const SurvivorId&) = default;
};

/// Un indice : son rang dans data/story.txt.
struct ClueId {
    std::uint16_t value = 0;

    friend auto operator<=>(const ClueId&, const ClueId&) = default;
};

inline constexpr std::size_t kMaxSurvivors = 4; ///< un joueur par survivant : 4 par monde
inline constexpr std::uint8_t kStepCount = 8;   ///< étapes du fil des révélations (ZONES.md)

struct SurvivorDef {
    SurvivorId id;
    std::string key;     ///< identifiant stable : `mara`
    std::string name;    ///< nom affiché : « Mara Velin »
    std::string capsule; ///< sa capsule au réveil : « C-7 »
    std::string awakening; ///< bribes de mémoire au réveil (mémoire floue), lignes séparées par \n
};

/// Une étape du fil des révélations et la phrase ajoutée au journal quand elle est comprise.
struct StepDef {
    std::uint8_t number = 0; ///< 1 à kStepCount
    std::string text;
};

struct ClueDef {
    ClueId id;
    std::string key;       ///< identifiant stable : `z1_capsule_log` (sauvegardes, journal)
    std::uint8_t zone = 1; ///< 1 à 5
    std::uint8_t step = 1; ///< étape du fil qu'il concerne
    bool reveals = true;   ///< faux : pressentiment, il prépare l'étape sans la débloquer
    std::optional<SurvivorId> owner; ///< indice personnel ; vide : commun
    std::string title;
    std::string text;    ///< lignes séparées par \n
    std::string memory;  ///< personnel : souvenir rendu à son survivant
    std::string generic; ///< personnel : version lue quand son survivant n'est joué par personne
    /// Où il est posé, en mètres depuis le point d'apparition du monde (est, sud), en attendant les
    /// marqueurs de la carte (T038) : le serveur le pose au sol.
    std::array<std::int32_t, 2> offset{};

    [[nodiscard]] bool personal() const { return owner.has_value(); }
};

} // namespace odyssee::data
