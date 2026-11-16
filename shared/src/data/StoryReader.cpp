#include "StoryReader.hpp"

#include "DataValues.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <utility>

namespace odyssee::data {

namespace {

constexpr std::array<std::string_view, 1> kIntroKeys{"line"};
constexpr std::array<std::string_view, 3> kSurvivorKeys{"name", "capsule", "memory"};
constexpr std::array<std::string_view, 1> kSurvivorRepeatable{"memory"};
constexpr std::array<std::string_view, 2> kStepKeys{"number", "text"};
constexpr std::array<std::string_view, 9> kClueKeys{"zone", "step",   "reveals", "owner", "title",
                                                    "text", "memory", "generic", "at"};
constexpr std::array<std::string_view, 3> kClueRepeatable{"text", "memory", "generic"};
constexpr std::array<std::string_view, 5> kClueRequired{"zone", "step", "title", "text", "at"};

/// Toutes les lignes `key` du bloc, réunies (une par ligne de texte).
std::string joinedLines(const DataBlock& block, std::string_view key) {
    std::string text;
    for (const DataField& field : block.fields) {
        if (field.key == key) {
            text += (text.empty() ? "" : "\n") + std::string(field.value);
        }
    }
    return text;
}

std::optional<std::int32_t> parseSigned(std::string_view text) {
    std::int32_t value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value < -kMaxClueOffset ||
        value > kMaxClueOffset) {
        return std::nullopt;
    }
    return value;
}

/// Lit les blocs un par un, en gardant ce qui est déjà connu (survivants cités par les indices).
class StoryBlockReader {
  public:
    explicit StoryBlockReader(std::string_view file) : _file(file) {}

    std::optional<DataError> read(const DataBlock& block) {
        if (!isValidKey(block.id)) {
            return fail(block.line, "identifiant " + quoted(block.id) +
                                        " invalide (minuscules, chiffres et « _ »)");
        }
        if (block.kind == "intro") {
            return readIntro(block);
        }
        if (block.kind == "survivor") {
            return readSurvivor(block);
        }
        if (block.kind == "step") {
            return readStep(block);
        }
        if (block.kind == "clue") {
            return readClue(block);
        }
        return fail(block.line, "type de bloc " + quoted(block.kind) +
                                    " inattendu : intro, survivor, step ou clue attendu");
    }

    std::optional<DataError> finish() {
        if (_read.intro.empty()) {
            return fail(0, "bloc « [intro …] » manquant");
        }
        if (_read.survivors.empty()) {
            return fail(0, "aucun survivant défini");
        }
        std::sort(_read.steps.begin(), _read.steps.end(),
                  [](const StepDef& a, const StepDef& b) { return a.number < b.number; });
        return std::nullopt;
    }

    StoryRead take() { return std::move(_read); }

  private:
    std::optional<DataError> readIntro(const DataBlock& block) {
        if (auto error = checkBlockKeys(block, {kIntroKeys, kIntroKeys, kIntroKeys}, _file)) {
            return error;
        }
        if (!_read.intro.empty()) {
            return fail(block.line, "bloc « [intro …] » répété");
        }
        for (const DataField& field : block.fields) {
            _read.intro.emplace_back(field.value);
        }
        return std::nullopt;
    }

    std::optional<DataError> readSurvivor(const DataBlock& block) {
        if (auto error =
                checkBlockKeys(block, {kSurvivorKeys, kSurvivorRepeatable, kSurvivorKeys}, _file)) {
            return error;
        }
        if (findSurvivor(block.id) || _read.survivors.size() >= kMaxSurvivors) {
            return fail(block.line,
                        "survivant " + quoted(block.id) + " déjà défini, ou plus de 4 survivants");
        }
        SurvivorDef survivor;
        survivor.id = SurvivorId{static_cast<std::uint8_t>(_read.survivors.size())};
        survivor.key = std::string(block.id);
        survivor.name = std::string(findField(block, "name")->value);
        survivor.capsule = std::string(findField(block, "capsule")->value);
        survivor.awakening = joinedLines(block, "memory");
        _read.survivors.push_back(std::move(survivor));
        return std::nullopt;
    }

    std::optional<DataError> readStep(const DataBlock& block) {
        if (auto error = checkBlockKeys(block, {kStepKeys, {}, kStepKeys}, _file)) {
            return error;
        }
        const DataField& numberField = *findField(block, "number");
        const auto number = parseNumber(numberField.value, 1, kStepCount);
        if (!number) {
            return fail(numberField.line, "numéro d'étape " + quoted(numberField.value) +
                                              " invalide : 1 à 8 attendu");
        }
        if (std::any_of(_read.steps.begin(), _read.steps.end(),
                        [&](const StepDef& step) { return step.number == *number; })) {
            return fail(numberField.line, "étape " + quoted(numberField.value) + " répétée");
        }
        _read.steps.push_back(
            {static_cast<std::uint8_t>(*number), std::string(findField(block, "text")->value)});
        return std::nullopt;
    }

    std::optional<DataError> readClue(const DataBlock& block) {
        if (auto error =
                checkBlockKeys(block, {kClueKeys, kClueRepeatable, kClueRequired}, _file)) {
            return error;
        }
        if (findClue(block.id) || _read.clues.size() >= kMaxClues) {
            return fail(block.line,
                        "indice " + quoted(block.id) + " déjà défini, ou trop d'indices");
        }
        ClueDef clue;
        clue.id = ClueId{static_cast<std::uint16_t>(_read.clues.size())};
        clue.key = std::string(block.id);
        if (auto error = readClueNumbers(block, clue)) {
            return error;
        }
        if (auto error = readClueTexts(block, clue)) {
            return error;
        }
        _read.clues.push_back(std::move(clue));
        return std::nullopt;
    }

    std::optional<DataError> readClueNumbers(const DataBlock& block, ClueDef& clue) const {
        const DataField& zone = *findField(block, "zone");
        const DataField& step = *findField(block, "step");
        const auto zoneNumber = parseNumber(zone.value, 1, 5);
        const auto stepNumber = parseNumber(step.value, 1, kStepCount);
        if (!zoneNumber || !stepNumber) {
            return fail(zone.line, "zone (1 à 5) ou étape (1 à 8) invalide");
        }
        clue.zone = static_cast<std::uint8_t>(*zoneNumber);
        clue.step = static_cast<std::uint8_t>(*stepNumber);
        if (const DataField* reveals = findField(block, "reveals")) {
            if (reveals->value != "yes" && reveals->value != "no") {
                return fail(reveals->line, "reveals : yes ou no attendu");
            }
            clue.reveals = reveals->value == "yes";
        }
        return readOffset(*findField(block, "at"), clue);
    }

    std::optional<DataError> readOffset(const DataField& field, ClueDef& clue) const {
        const std::size_t space = field.value.find(' ');
        const auto east = parseSigned(field.value.substr(0, space));
        const auto south = space == std::string_view::npos
                               ? std::nullopt
                               : parseSigned(field.value.substr(space + 1));
        if (!east || !south) {
            return fail(field.line, "position " + quoted(field.value) +
                                        " invalide : « est sud » en mètres entiers attendu");
        }
        clue.offset = {*east, *south};
        return std::nullopt;
    }

    std::optional<DataError> readClueTexts(const DataBlock& block, ClueDef& clue) const {
        clue.title = std::string(findField(block, "title")->value);
        clue.text = joinedLines(block, "text");
        clue.memory = joinedLines(block, "memory");
        clue.generic = joinedLines(block, "generic");
        if (clue.title.size() > kMaxNameLength ||
            std::max({clue.text.size(), clue.memory.size(), clue.generic.size()}) >
                kMaxStoryTextLength) {
            return fail(block.line, "titre ou texte trop long");
        }
        return readOwner(block, clue);
    }

    /// `owner = survivant` (facultatif) : un indice personnel a un souvenir et une version
    /// générique.
    std::optional<DataError> readOwner(const DataBlock& block, ClueDef& clue) const {
        const DataField* owner = findField(block, "owner");
        if (!owner) {
            return std::nullopt;
        }
        const SurvivorDef* survivor = findSurvivor(owner->value);
        if (!survivor) {
            return fail(owner->line, "survivant " + quoted(owner->value) +
                                         " inconnu (à définir avant les indices)");
        }
        if (clue.memory.empty() || clue.generic.empty()) {
            return fail(owner->line, "un indice personnel demande « memory » et « generic »");
        }
        clue.owner = survivor->id;
        return std::nullopt;
    }

    [[nodiscard]] const SurvivorDef* findSurvivor(std::string_view key) const {
        const auto found = std::find_if(_read.survivors.begin(), _read.survivors.end(),
                                        [key](const SurvivorDef& s) { return s.key == key; });
        return found == _read.survivors.end() ? nullptr : &*found;
    }

    [[nodiscard]] bool findClue(std::string_view key) const {
        return std::any_of(_read.clues.begin(), _read.clues.end(),
                           [key](const ClueDef& c) { return c.key == key; });
    }

    [[nodiscard]] DataError fail(std::size_t line, std::string message) const {
        return makeError(_file, line, std::move(message));
    }

    std::string_view _file;
    StoryRead _read;
};

} // namespace

StoryRead readStory(const std::vector<DataBlock>& blocks, std::string_view file) {
    StoryBlockReader reader(file);
    for (const DataBlock& block : blocks) {
        if (auto error = reader.read(block)) {
            StoryRead failed;
            failed.error = std::move(error);
            return failed;
        }
    }
    if (auto error = reader.finish()) {
        StoryRead failed;
        failed.error = std::move(error);
        return failed;
    }
    return reader.take();
}

} // namespace odyssee::data
