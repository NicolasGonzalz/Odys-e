// L'histoire du jeu (T049) : data/story.txt et sa lecture.
#include "odyssee/data/StoryCatalog.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <string_view>

using namespace odyssee::data;

namespace {

const std::string kDataDir = std::string(ODYSSEE_SOURCE_DIR) + "/data";

constexpr std::string_view kMinimal = R"([intro capsule]
line = Capsule {capsule}

[survivor mara]
name = Mara Velin
capsule = C-7
memory = Je m'appelle Mara.

[step s1]
number = 1
text = Je me suis réveillé seul.

[clue z1_note]
support = letter
date = 2074-10-17
zone = 1
step = 1
title = Note
text = Première ligne.
text = Deuxième ligne.
at = 3 -4

[clue z1_perso]
support = data
date = 2073
zone = 1
step = 2
reveals = no
owner = mara
title = Fiche
text = Velin, M.
memory = Je devais partir.
generic = Une fiche.
at = 0 0
)";

std::string errorOf(std::string_view text) {
    const auto result = StoryCatalog::fromText(text);
    REQUIRE_FALSE(result.catalog);
    REQUIRE(result.error);
    return result.error->describe();
}

} // namespace

TEST_CASE("Histoire : le vrai data/story.txt se charge, cohérent avec docs/HISTOIRE.md") {
    const auto result = StoryCatalog::fromDirectory(kDataDir);
    REQUIRE_MESSAGE(result.catalog, (result.error ? result.error->describe() : ""));
    const StoryCatalog& story = *result.catalog;

    CHECK(story.survivors().size() == 4);
    CHECK(story.steps().size() == kStepCount);
    CHECK(story.clues().size() == 27);
    for (const ClueDef& clue : story.clues()) {
        CHECK(story.clue(clue.id) == &clue); // indice = rang dans le fichier
        CHECK(story.step(clue.step) != nullptr);
    }
    // Chaque survivant a au moins un indice personnel par zone de la bêta.
    for (const SurvivorDef& survivor : story.survivors()) {
        for (std::uint8_t zone = 1; zone <= 3; ++zone) {
            CHECK(std::any_of(story.clues().begin(), story.clues().end(), [&](const ClueDef& c) {
                return c.owner == survivor.id && c.zone == zone;
            }));
        }
    }
    const auto intro = story.introFor(story.findSurvivor("teo")->id);
    CHECK(intro[1].find("C-9") != std::string::npos);
}

TEST_CASE("Histoire : un fichier minimal est lu entièrement") {
    const auto result = StoryCatalog::fromText(kMinimal);
    REQUIRE_MESSAGE(result.catalog, (result.error ? result.error->describe() : ""));
    const StoryCatalog& story = *result.catalog;

    const ClueDef* note = story.findClue("z1_note");
    REQUIRE(note != nullptr);
    CHECK(note->text == "Première ligne.\nDeuxième ligne.");
    CHECK(note->offset == std::array<std::int32_t, 2>{3, -4});
    CHECK_FALSE(note->personal());
    CHECK(note->reveals);
    CHECK(note->support == ClueSupport::Letter);
    CHECK(note->date == "2074-10-17");

    const ClueDef* personal = story.findClue("z1_perso");
    REQUIRE(personal != nullptr);
    CHECK(personal->owner == story.findSurvivor("mara")->id);
    CHECK_FALSE(personal->reveals);
    CHECK(personal->generic == "Une fiche.");
    CHECK(story.introFor(SurvivorId{0}).front() == "Capsule C-7");
}

TEST_CASE("Histoire : erreurs, avec leur ligne") {
    std::string text(kMinimal);
    SUBCASE("survivant inconnu") {
        text.replace(text.find("owner = mara"), 12, "owner = zoe");
        CHECK(errorOf(text).find("inconnu") != std::string::npos);
    }
    SUBCASE("indice personnel sans version générique") {
        text.erase(text.find("generic = Une fiche.\n"), 21);
        CHECK(errorOf(text).find("generic") != std::string::npos);
    }
    SUBCASE("position invalide") {
        text.replace(text.find("at = 3 -4"), 9, "at = 3");
        CHECK(errorOf(text).find("position") != std::string::npos);
    }
    SUBCASE("date invalide") {
        text.replace(text.find("date = 2073"), 11, "date = 2073-13");
        CHECK(errorOf(text).find("date") != std::string::npos);
    }
    SUBCASE("support inconnu") {
        text.replace(text.find("support = data"), 14, "support = video");
        CHECK(errorOf(text).find("support") != std::string::npos);
    }
    SUBCASE("étape hors limites") {
        text.replace(text.find("number = 1"), 10, "number = 9");
        CHECK(errorOf(text).find("étape") != std::string::npos);
    }
    SUBCASE("pas d'intro") {
        text.erase(0, text.find("[survivor"));
        CHECK(errorOf(text).find("intro") != std::string::npos);
    }
}
