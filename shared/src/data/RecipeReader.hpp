#pragma once

// Lecture interne de data/recipes.txt : blocs « [recipe …] » → RecipeDef, avec vérification des
// objets cités (lus avant, dans items.txt).

#include "ItemReader.hpp"
#include "odyssee/data/DataError.hpp"
#include "odyssee/data/DataText.hpp"
#include "odyssee/data/RecipeDef.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace odyssee::data {

inline constexpr std::size_t kMaxRecipeInputs = 8;
inline constexpr std::uint32_t kMaxCraftTicks = 20 * 60 * 10; ///< 10 minutes

struct RecipesRead {
    std::vector<RecipeDef> recipes;
    std::optional<DataError> error;
};

/// Interprète les blocs de recettes ; `items` donne les objets connus, `file` sert aux erreurs.
[[nodiscard]] RecipesRead readRecipes(const std::vector<DataBlock>& blocks, const ItemsRead& items,
                                      std::string_view file);

} // namespace odyssee::data
