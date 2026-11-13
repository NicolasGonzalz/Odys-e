#pragma once

#include "odyssee/data/DataError.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace odyssee::data {

/// Format texte commun aux fichiers de données (docs/DATA_FORMAT.md) :
///
///     # commentaire
///     [item stone_axe]
///     name = Hache de pierre
///     stack = 1
///
/// Ce découpage ne connaît ni les objets ni les recettes : il rend des blocs de lignes
/// « clé = valeur », que la lecture des objets et des recettes interprète ensuite. Le texte est
/// hostile : aucune lecture hors limites, toute anomalie devient une DataError.

inline constexpr std::size_t kMaxLineLength = 1024; ///< octets, fin de ligne exclue
inline constexpr std::size_t kMaxFieldsPerBlock = 64;
inline constexpr std::size_t kMaxBlocks = 4096;

/// Une ligne « clé = valeur » (sans les espaces autour). Vues sur le texte d'origine.
struct DataField {
    std::string_view key;
    std::string_view value;
    std::size_t line = 0;
};

/// Un bloc « [kind id] » et ses lignes.
struct DataBlock {
    std::string_view kind;
    std::string_view id;
    std::size_t line = 0;
    std::vector<DataField> fields;
};

struct DataBlocks {
    std::vector<DataBlock> blocks;
    std::optional<DataError> error; ///< renseigné si le texte est refusé (blocs alors incomplets)
};

/// Découpe `text` en blocs. `fileName` sert aux messages d'erreur. Les vues rendues pointent dans
/// `text`, qui doit donc vivre au moins aussi longtemps que le résultat.
[[nodiscard]] DataBlocks parseDataText(std::string_view text, std::string_view fileName);

} // namespace odyssee::data
