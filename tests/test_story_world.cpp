// L'histoire dans le monde (T049) : survivants, lecture des indices, journal commun, sauvegarde.
#include "GameWorld.hpp"
#include "persistence/SaveStore.hpp"
#include "story/ClueReading.hpp"
#include "story/SurvivorRoster.hpp"

#include "odyssee/data/StoryCatalog.hpp"
#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

using namespace odyssee;
using odyssee::server::ConnectedPlayer;
using odyssee::server::GameWorld;
using odyssee::server::SurvivorRoster;

namespace {

/// Le joueur 0 apparaît 3 m à l'ouest du point d'apparition (GameWorld::spawn) : les indices sont
/// posés 1 m au nord (la note) ou au sud (la fiche) de lui, ou loin.
constexpr std::string_view kStory = R"([intro capsule]
line = Capsule {capsule}

[survivor mara]
name = Mara Velin
capsule = C-7
memory = Mara.

[survivor teo]
name = Teo Brandt
capsule = C-9
memory = Teo.

[clue note]
support = letter
date = 2074
zone = 1
step = 1
title = Note
text = Commun.
at = -3 -1

[clue mara_file]
support = letter
date = 2074
zone = 1
step = 2
owner = mara
title = Fiche
text = Velin, M.
memory = Je devais partir.
generic = Une fiche.
at = -3 1

[clue far]
support = letter
date = 2074
zone = 2
step = 3
title = Loin
text = Loin.
at = 200 0
)";

data::StoryCatalog story() {
    auto result = data::StoryCatalog::fromText(kStory);
    REQUIRE_MESSAGE(result.catalog, (result.error ? result.error->describe() : ""));
    return std::move(*result.catalog);
}

const data::SurvivorId kMara{0}, kTeo{1};

GameWorld worldWith(std::vector<ConnectedPlayer> players) {
    GameWorld world;
    CHECK(world.useStory(story()) == 3);
    world.syncPlayers(players);
    return world;
}

/// Un clic gauche du joueur vers `yawDegrees` (270 : nord, 90 : sud), au tick `tick` (relâché
/// au tick suivant).
void clickToward(GameWorld& world, net::PlayerId player, net::Tick tick, float yawDegrees) {
    net::InputCommand command;
    command.yaw = net::quantizeYaw(yawDegrees);
    command.buttons = static_cast<std::uint16_t>(net::InputButton::Primary);
    world.receiveInputs(player, net::InputBatch{tick, {command}});
    world.step();
    command.buttons = 0;
    world.receiveInputs(player, net::InputBatch{tick + 1, {command}});
    world.step();
}

template <typename Event> std::vector<Event> eventsOf(GameWorld& world, net::PlayerId player) {
    std::vector<Event> found;
    for (const auto& event : world.takeEvents(player)) {
        if (const auto* typed = std::get_if<Event>(&event)) {
            found.push_back(*typed);
        }
    }
    return found;
}

} // namespace

TEST_CASE("Indices : qui lit quoi (règles de docs/HISTOIRE.md)") {
    const data::StoryCatalog s = story();
    const data::ClueDef& common = *s.findClue("note");
    const data::ClueDef& personal = *s.findClue("mara_file");
    SurvivorRoster roster;

    CHECK(server::readClue(common, std::nullopt, roster).version == net::ClueVersion::Full);
    // Mara n'est jouée par personne : la version générique, pour tous.
    CHECK(server::readClue(personal, kTeo, roster).version == net::ClueVersion::Generic);

    REQUIRE(roster.assign(kMara, "Nicolas"));
    const auto byMara = server::readClue(personal, kMara, roster);
    CHECK(byMara.version == net::ClueVersion::Full);
    CHECK(byMara.ownerOnly);
    const auto byTeo = server::readClue(personal, kTeo, roster);
    CHECK_FALSE(byTeo.version);
    CHECK(byTeo.lockedFor == kMara);
}

TEST_CASE("Survivants : un joueur par survivant, un survivant par joueur") {
    SurvivorRoster roster;
    CHECK(roster.assign(kMara, "Nicolas"));
    CHECK_FALSE(roster.assign(kMara, "Lina"));   // déjà pris
    CHECK_FALSE(roster.assign(kTeo, "Nicolas")); // il en a déjà un
    CHECK(roster.assign(kTeo, "Lina"));
    CHECK(roster.survivorOf("Lina") == kTeo);
    CHECK(roster.takenMask() == 0b11);
}

TEST_CASE("Monde : choisir son survivant ; l'écran du réveil, puis le statut chez les autres") {
    GameWorld world = worldWith({{0, 1, "Nicolas"}, {1, 2, "Lina"}});
    CHECK(eventsOf<net::SurvivorStatus>(world, 0).front() == net::SurvivorStatus{0xFF, 0, false});
    (void)world.takeEvents(1);

    world.receiveRequest(0, net::ChooseSurvivor{kMara.value});
    const auto mine = eventsOf<net::SurvivorStatus>(world, 0);
    REQUIRE(!mine.empty());
    CHECK(mine.back() == net::SurvivorStatus{kMara.value, 0b01, true});
    CHECK(eventsOf<net::SurvivorStatus>(world, 1).back() == net::SurvivorStatus{0xFF, 0b01, false});

    world.receiveRequest(1, net::ChooseSurvivor{kMara.value}); // déjà prise : refusé
    CHECK(eventsOf<net::SurvivorStatus>(world, 1).back().yours == 0xFF);
    CHECK(world.survivorOf(0) == kMara);
}

TEST_CASE("Monde : un indice commun lu au clic entre dans le journal de tous") {
    GameWorld world = worldWith({{0, 1, "Nicolas"}, {1, 2, "Lina"}});
    (void)world.takeEvents(0);
    (void)world.takeEvents(1);

    clickToward(world, 0, 1, 270.0f); // la note, au nord
    const auto entries = eventsOf<net::JournalEntry>(world, 1);
    REQUIRE(entries.size() == 1);
    CHECK(entries[0].clue == 0);
    CHECK(world.journal().records().size() == 1);

    (void)world.takeEvents(0);
    clickToward(world, 0, 3, 90.0f); // la fiche de Mara, que personne ne joue : générique
    REQUIRE(eventsOf<net::JournalEntry>(world, 1).size() == 1);
    CHECK(world.journal().records().back().version == net::ClueVersion::Generic);

    const auto snapshot = world.snapshotFor(1, 1);
    const auto clue =
        std::find_if(snapshot.entities.begin(), snapshot.entities.end(),
                     [](const net::EntityState& e) { return e.kind == net::EntityKind::Clue; });
    REQUIRE(clue != snapshot.entities.end());
    CHECK((clue->flags & net::kClueRead) != 0);
    CHECK(std::count_if(snapshot.entities.begin(), snapshot.entities.end(), [](const auto& e) {
              return e.kind == net::EntityKind::Clue;
          }) == 2); // le lointain est hors de portée
}

TEST_CASE("Monde : l'indice personnel d'un survivant joué par un autre est verrouillé") {
    GameWorld world = worldWith({{0, 1, "Nicolas"}, {1, 2, "Lina"}});
    world.receiveRequest(0, net::ChooseSurvivor{kTeo.value});
    world.receiveRequest(1, net::ChooseSurvivor{kMara.value});
    (void)world.takeEvents(0);
    (void)world.takeEvents(1);

    clickToward(world, 0, 1, 90.0f); // Nicolas (Teo) vise la fiche de Mara, jouée par Lina
    const auto locked = eventsOf<net::ClueLocked>(world, 0);
    REQUIRE(locked.size() == 1);
    CHECK(locked[0].owner == kMara.value);
    CHECK(world.journal().records().empty());
    CHECK(eventsOf<net::JournalEntry>(world, 1).empty());
}

TEST_CASE("Monde : survivants et journal survivent à une sauvegarde") {
    GameWorld before = worldWith({{0, 1, "Nicolas"}});
    before.receiveRequest(0, net::ChooseSurvivor{kMara.value});
    clickToward(before, 0, 1, 270.0f);
    clickToward(before, 0, 3, 90.0f);
    REQUIRE(before.journal().records().size() == 2); // note + fiche complète (Mara la lit)

    const auto path =
        (std::filesystem::temp_directory_path() / "odyssee_test_histoire.sqlite").string();
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    {
        auto store = server::SaveStore::open(path);
        REQUIRE(store);
        REQUIRE(store->store(before.save()));
    }
    std::optional<server::WorldSave> save;
    {
        auto store = server::SaveStore::open(path);
        REQUIRE(store);
        save = store->load();
    }
    std::filesystem::remove(path, ignored);
    REQUIRE(save);
    CHECK(save->journal.size() == 2);

    GameWorld after;
    CHECK(after.useStory(story()) == 3);
    after.load(*save);
    after.syncPlayers(std::vector<ConnectedPlayer>{{0, 1, "Nicolas"}});
    CHECK(after.survivorOf(0) == kMara);
    const auto was = before.journal().records();
    const auto now = after.journal().records();
    CHECK(std::equal(was.begin(), was.end(), now.begin(), now.end()));
    CHECK(eventsOf<net::JournalEntry>(after, 0).size() == 2); // tout son journal, à la connexion
}
