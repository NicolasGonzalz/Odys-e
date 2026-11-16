#include "odyssee/data/StoryCatalog.hpp"

#include "StoryReader.hpp"
#include "TextFile.hpp"
#include "odyssee/data/DataText.hpp"

#include <algorithm>
#include <utility>

namespace odyssee::data {

namespace {

StoryCatalogResult failure(DataError error) {
    return {std::nullopt, std::move(error)};
}

template <typename Def, typename Key> const Def* findByKey(const std::vector<Def>& defs, Key key) {
    const auto found =
        std::find_if(defs.begin(), defs.end(), [key](const Def& def) { return def.key == key; });
    return found == defs.end() ? nullptr : &*found;
}

} // namespace

StoryCatalogResult StoryCatalog::fromText(std::string_view text) {
    return build(text, "story.txt");
}

StoryCatalogResult StoryCatalog::fromFile(const std::string& path) {
    const auto text = readTextFile(path);
    if (!text) {
        return failure(DataError{path, 0,
                                 "impossible de lire le fichier (absent, illisible ou plus de " +
                                     std::to_string(kMaxDataFileSize / 1024) + " Kio)"});
    }
    return build(*text, path);
}

StoryCatalogResult StoryCatalog::fromDirectory(const std::string& dataDir) {
    return fromFile(dataDir + "/story.txt");
}

StoryCatalogResult StoryCatalog::build(std::string_view text, std::string_view file) {
    const DataBlocks blocks = parseDataText(text, file);
    if (blocks.error) {
        return failure(*blocks.error);
    }
    StoryRead read = readStory(blocks.blocks, file);
    if (read.error) {
        return failure(*read.error);
    }
    StoryCatalog catalog;
    catalog._intro = std::move(read.intro);
    catalog._survivors = std::move(read.survivors);
    catalog._steps = std::move(read.steps);
    catalog._clues = std::move(read.clues);
    return {std::move(catalog), std::nullopt};
}

const SurvivorDef* StoryCatalog::survivor(SurvivorId id) const {
    return id.value < _survivors.size() ? &_survivors[id.value] : nullptr;
}

const SurvivorDef* StoryCatalog::findSurvivor(std::string_view key) const {
    return findByKey(_survivors, key);
}

const ClueDef* StoryCatalog::clue(ClueId id) const {
    return id.value < _clues.size() ? &_clues[id.value] : nullptr;
}

const ClueDef* StoryCatalog::findClue(std::string_view key) const {
    return findByKey(_clues, key);
}

const StepDef* StoryCatalog::step(std::uint8_t number) const {
    const auto found = std::find_if(_steps.begin(), _steps.end(), [number](const StepDef& step) {
        return step.number == number;
    });
    return found == _steps.end() ? nullptr : &*found;
}

std::vector<std::string> StoryCatalog::introFor(SurvivorId id) const {
    const SurvivorDef* who = survivor(id);
    std::vector<std::string> lines;
    for (std::string line : _intro) {
        const std::size_t at = line.find("{capsule}");
        if (at != std::string::npos && who) {
            line.replace(at, 9, who->capsule);
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

} // namespace odyssee::data
