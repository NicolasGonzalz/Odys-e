#pragma once

#include "odyssee/data/DataError.hpp"
#include "odyssee/data/StoryDef.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

struct StoryCatalogResult;

/// L'histoire du jeu (data/story.txt, T049), chargée et vérifiée : survivants, écran du réveil,
/// étapes du fil des révélations, indices.
class StoryCatalog {
  public:
    /// Charge depuis du texte en mémoire (nom « story.txt » dans les erreurs).
    [[nodiscard]] static StoryCatalogResult fromText(std::string_view text);
    [[nodiscard]] static StoryCatalogResult fromFile(const std::string& path);
    /// Charge story.txt dans le dossier `dataDir`.
    [[nodiscard]] static StoryCatalogResult fromDirectory(const std::string& dataDir);

    /// Lignes de l'écran de la capsule ; « {capsule} » y est remplacé par celle du survivant.
    [[nodiscard]] std::span<const std::string> intro() const { return _intro; }
    [[nodiscard]] std::span<const SurvivorDef> survivors() const { return _survivors; }
    [[nodiscard]] std::span<const StepDef> steps() const { return _steps; }
    [[nodiscard]] std::span<const ClueDef> clues() const { return _clues; }

    [[nodiscard]] const SurvivorDef* survivor(SurvivorId id) const;
    [[nodiscard]] const SurvivorDef* findSurvivor(std::string_view key) const;
    [[nodiscard]] const ClueDef* clue(ClueId id) const;
    [[nodiscard]] const ClueDef* findClue(std::string_view key) const;
    /// Phrase du journal de l'étape `number` (nullptr si elle n'est pas définie).
    [[nodiscard]] const StepDef* step(std::uint8_t number) const;

    /// L'écran du réveil d'un survivant, prêt à afficher.
    [[nodiscard]] std::vector<std::string> introFor(SurvivorId id) const;

  private:
    static StoryCatalogResult build(std::string_view text, std::string_view file);

    std::vector<std::string> _intro;
    std::vector<SurvivorDef> _survivors;
    std::vector<StepDef> _steps;
    std::vector<ClueDef> _clues;
};

struct StoryCatalogResult {
    std::optional<StoryCatalog> catalog; ///< vide en cas d'erreur
    std::optional<DataError> error;
};

} // namespace odyssee::data
