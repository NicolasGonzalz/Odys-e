#pragma once
// Lecture interne de data/story.txt (T049) : blocs « [intro …] », « [survivor …] », « [step …] »
// et « [clue …] » → définitions de l'histoire, vérifiées.

#include "odyssee/data/DataError.hpp"
#include "odyssee/data/DataText.hpp"
#include "odyssee/data/StoryDef.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

inline constexpr std::size_t kMaxClues = 1024;
inline constexpr std::size_t kMaxStoryTextLength = 2000; ///< texte d'un indice, en octets
inline constexpr std::int32_t kMaxClueOffset = 100'000;  ///< mètres depuis le point d'apparition

struct StoryRead {
    std::vector<std::string> intro;
    std::vector<SurvivorDef> survivors; ///< indice = SurvivorId
    std::vector<StepDef> steps;         ///< dans l'ordre des numéros
    std::vector<ClueDef> clues;         ///< indice = ClueId
    std::optional<DataError> error;
};

/// Interprète les blocs de l'histoire ; `file` sert aux erreurs.
[[nodiscard]] StoryRead readStory(const std::vector<DataBlock>& blocks, std::string_view file);

} // namespace odyssee::data
