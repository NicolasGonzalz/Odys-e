#pragma once

#include "Inventory.hpp"

#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <cstdint>
#include <optional>

namespace odyssee::server {

/// Composant : la fabrication en cours d'un joueur (T047), une seule à la fois.
struct Crafting {
    std::uint16_t recipe = 0;    ///< rang de la recette dans data/recipes.txt
    std::uint32_t ticksLeft = 0; ///< 0 : rien en cours
};

/// Commence une fabrication : vérifie la recette, le poste et les ingrédients, puis retire les
/// ingrédients (ils sont « dans l'établi » pendant la fabrication). Le résultat dit au joueur
/// ce qui se passe ; seul Started a changé quelque chose.
///
/// Postes : seules les recettes « à la main » sont possibles tant que les établis et forges ne
/// sont pas posés dans le monde.
[[nodiscard]] net::CraftOutcome startCraft(Crafting& crafting, Inventory& inventory,
                                           std::uint16_t recipe, const data::Catalog& catalog);

/// Un tick de fabrication. Quand elle finit, le résultat est mis dans l'inventaire (ce qui n'y
/// tient pas est perdu) et la recette terminée est renvoyée.
[[nodiscard]] std::optional<std::uint16_t> stepCraft(Crafting& crafting, Inventory& inventory,
                                                     const data::Catalog& catalog);

} // namespace odyssee::server
