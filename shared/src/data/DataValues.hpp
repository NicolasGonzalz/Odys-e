#pragma once

// Outils internes de lecture des fichiers de données : valeurs simples et contrôle des clés d'un
// bloc. Pas dans include/ : seuls ItemReader et RecipeReader s'en servent.

#include "odyssee/data/DataError.hpp"
#include "odyssee/data/DataText.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace odyssee::data {

inline constexpr std::size_t kMaxKeyLength = 48;        ///< identifiant texte, en octets
inline constexpr std::size_t kMaxNameLength = 96;       ///< nom affiché, en octets (UTF-8)
inline constexpr std::size_t kMaxModelPathLength = 128; ///< chemin de modèle, en octets

/// « texte » entre guillemets français, pour les messages.
[[nodiscard]] std::string quoted(std::string_view text);

[[nodiscard]] DataError makeError(std::string_view file, std::size_t line, std::string message);

/// Entier décimal positif écrit en entier (« 12 », pas « 12a » ni « -1 ») et compris dans
/// [min, max] ; nullopt sinon.
[[nodiscard]] std::optional<std::uint32_t> parseNumber(std::string_view text, std::uint32_t min,
                                                       std::uint32_t max);

/// Identifiant texte : minuscule en tête, puis minuscules, chiffres ou « _ » (`stone_axe`).
[[nodiscard]] bool isValidKey(std::string_view text);

/// Chemin de modèle : segments d'identifiant séparés par « / » (`outils/hache_pierre`).
[[nodiscard]] bool isValidModelPath(std::string_view text);

/// Clés admises dans un bloc : `allowed` toutes, `repeatable` celles qui peuvent revenir,
/// `required` celles qui doivent apparaître. Première anomalie trouvée, ou nullopt.
struct BlockKeys {
    std::span<const std::string_view> allowed;
    std::span<const std::string_view> repeatable;
    std::span<const std::string_view> required;
};
[[nodiscard]] std::optional<DataError> checkBlockKeys(const DataBlock& block, const BlockKeys& keys,
                                                      std::string_view file);

/// Première ligne de clé `key` du bloc (nullptr si absente).
[[nodiscard]] const DataField* findField(const DataBlock& block, std::string_view key);

} // namespace odyssee::data
