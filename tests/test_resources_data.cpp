// Types de ressources récoltables (data/resources.txt, docs/DATA_FORMAT.md), entités ressources
// dans les snapshots, et leur affichage côté client (T044).
#include "odyssee/client/Interpolation.hpp"
#include "odyssee/core/Config.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

using namespace odyssee;
using namespace odyssee::data;

namespace {

const std::string kDataDir = std::string(ODYSSEE_SOURCE_DIR) + "/data";

constexpr std::string_view kItems = R"([item wood_log]
name = Bûche
stack = 20
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
)";

Catalog testItems() {
    CatalogResult result = Catalog::fromText(kItems, "");
    REQUIRE(result.catalog.has_value());
    return std::move(*result.catalog);
}

/// Un arbre valide ; `extra` s'ajoute à la fin du bloc (ligne 9 et suivantes).
std::string tree(std::string_view item = "wood_log 2", std::string_view extra = "") {
    return "[resource tree]\nname = Arbre\nshape = tree\nitem = " + std::string(item) +
           "\nhits = 5\nhit_ticks = 10\nregrow = 6000\nabundance = 3\n" + std::string(extra);
}

/// Erreur attendue au chargement (échec du test si le chargement réussit).
DataError resourcesError(std::string_view text) {
    const Catalog items = testItems();
    ResourceCatalogResult result = ResourceCatalog::fromText(text, items);
    REQUIRE_FALSE(result.catalog.has_value());
    REQUIRE(result.error.has_value());
    return *result.error;
}

net::EntityState resource(net::EntityId id, std::uint8_t flags) {
    return net::EntityState{id, net::EntityKind::Resource, {512, -256, 2560}, 1234, flags};
}

} // namespace

TEST_CASE("Ressources : le vrai data/resources.txt se charge") {
    const CatalogResult items = Catalog::fromDirectory(kDataDir);
    REQUIRE(items.catalog.has_value());
    const ResourceCatalogResult result = ResourceCatalog::fromDirectory(kDataDir, *items.catalog);
    REQUIRE_MESSAGE(result.catalog.has_value(), (result.error ? result.error->describe() : ""));

    const ResourceCatalog& types = *result.catalog;
    CHECK(types.types().size() >= 4);
    const ResourceDef* treeType = types.find("tree");
    REQUIRE(treeType != nullptr);
    CHECK(treeType->shape == ResourceShape::Tree);
    CHECK(treeType->yield.item == *items.catalog->findItem("wood_log"));
    CHECK(treeType->handAllowed);
    CHECK(treeType->tools.front() == *items.catalog->findItem("stone_axe"));
    CHECK(types.find("rock") != nullptr);
    CHECK(types.find("ore_vein") != nullptr);
    CHECK_FALSE(types.find("ore_vein")->handAllowed);
    for (const ResourceDef& type : types.types()) {
        CHECK(types.type(type.id) == &type); // indice = rang dans le fichier
    }
}

TEST_CASE("Ressources : un bloc complet est lu") {
    const Catalog items = testItems();
    const auto result = ResourceCatalog::fromText(tree("wood_log 2", "tool = stone_axe\n"), items);
    REQUIRE(result.catalog.has_value());
    const ResourceDef& type = result.catalog->types()[0];
    CHECK(type.id.value == 0);
    CHECK(type.key == "tree");
    CHECK(type.name == "Arbre");
    CHECK(type.yield == ItemStack{*items.findItem("wood_log"), 2});
    CHECK(type.hits == 5);
    CHECK(type.hitTicks == 10);
    CHECK(type.regrowTicks == 6000);
    CHECK(type.abundance == 3);
    REQUIRE(type.tools.size() == 1);
    CHECK(type.tools[0] == *items.findItem("stone_axe"));
    CHECK(type.handAllowed);
    CHECK(result.catalog->type(ResourceTypeId{1}) == nullptr);
    CHECK(result.catalog->find("rock") == nullptr);
}

TEST_CASE("Ressources : erreurs, avec fichier et ligne") {
    SUBCASE("objet inconnu") {
        const DataError error = resourcesError(tree("wood_lgo 2"));
        CHECK(error.describe() ==
              "resources.txt:4 : objet inconnu « wood_lgo » (absent de items.txt)");
    }
    SUBCASE("quantité au-delà de la pile de l'objet") {
        CHECK(resourcesError(tree("wood_log 21")).line == 4);
    }
    SUBCASE("quantité absente") {
        CHECK(resourcesError(tree("wood_log")).line == 4);
    }
    SUBCASE("outil qui n'en est pas un") {
        const DataError error = resourcesError(tree("wood_log 1", "tool = wood_log\n"));
        CHECK(error.line == 9);
        CHECK(error.message == "l'objet « wood_log » n'est pas un outil");
    }
    SUBCASE("outil répété") {
        CHECK(resourcesError(tree("wood_log 1", "tool = stone_axe\ntool = stone_axe\n")).line ==
              10);
    }
    SUBCASE("hand = no sans outil") {
        CHECK(resourcesError(tree("wood_log 1", "hand = no\n")).line == 9);
    }
    SUBCASE("hand ni yes ni no") {
        CHECK(resourcesError(tree("wood_log 1", "tool = stone_axe\nhand = non\n")).line == 10);
    }
    SUBCASE("forme inconnue") {
        std::string text = tree();
        text.replace(text.find("shape = tree"), 12, "shape = cone");
        CHECK(resourcesError(text).line == 3);
    }
    SUBCASE("nombres hors bornes") {
        std::string text = tree();
        text.replace(text.find("hits = 5"), 8, "hits = 0");
        CHECK(resourcesError(text).line == 5);
        text = tree();
        text.replace(text.find("regrow = 6000"), 13, "regrow = -1");
        CHECK(resourcesError(text).line == 7);
    }
    SUBCASE("clé obligatoire absente, clé inconnue, bloc d'un autre type") {
        std::string text = tree();
        text.replace(text.find("abundance = 3\n"), 14, "");
        CHECK(resourcesError(text).line == 1);
        CHECK(resourcesError(tree("wood_log 1", "colour = red\n")).line == 9);
        CHECK(resourcesError("[item tree]\nname = Arbre\n").line == 1);
    }
    SUBCASE("type en double") {
        CHECK(resourcesError(tree() + tree()).line == 9);
    }
    SUBCASE("fichier absent") {
        const Catalog items = testItems();
        const auto result = ResourceCatalog::fromFile(kDataDir + "/absent.txt", items);
        REQUIRE(result.error.has_value());
        CHECK(result.error->line == 0);
    }
}

TEST_CASE("Ressources : octets aléatoires dans le fichier") {
    std::mt19937_64 random(0x7E5500CE);
    const auto below = [&random](std::size_t bound) {
        return static_cast<std::size_t>(random() % bound);
    };
    using namespace std::string_view_literals;
    const std::string_view alphabet = "[]= \n#\tabz019_/\r\0\xC3"sv;
    const Catalog items = testItems();
    const std::string valid = tree("wood_log 2", "tool = stone_axe\nhand = no\n");

    for (int i = 0; i < 3'000; ++i) {
        std::string text = valid;
        for (std::size_t c = 1 + below(6); c > 0; --c) {
            text[below(text.size())] = alphabet[below(alphabet.size())];
        }
        const ResourceCatalogResult result = ResourceCatalog::fromText(text, items);
        CHECK(result.catalog.has_value() != result.error.has_value());
        if (result.catalog) {
            for (const ResourceDef& type : result.catalog->types()) {
                CHECK(type.hits >= 1);
                CHECK(type.yield.count >= 1);
                CHECK((type.handAllowed || !type.tools.empty()));
            }
        }
    }
}

TEST_CASE("Snapshot : les entités ressources passent le réseau") {
    const net::Snapshot snapshot{
        8,
        3,
        {resource(net::kFirstResourceEntity, 2), resource(77, 1 | net::kResourceDepleted)},
        {},
        {}};
    const auto type = static_cast<std::uint8_t>(net::GameMessageType::Snapshot);
    const auto decoded = net::decodeGameMessage(type, net::encodeGameMessage(snapshot));
    REQUIRE(decoded.has_value());
    CHECK(std::get<net::Snapshot>(*decoded) == snapshot);

    // Une sorte inconnue (3) est toujours refusée : kind est après ticks (8), self (8),
    // vitals (3), compteur (1) et id (2).
    auto bytes = net::encodeGameMessage(net::Snapshot{1, 1, {resource(9, 0)}, {}, {}});
    REQUIRE(bytes[22] == std::byte{2});
    bytes[22] = std::byte{3};
    CHECK_FALSE(net::decodeGameMessage(type, bytes).has_value());
}

TEST_CASE("Protocole : version 7 au moins depuis les ressources") {
    CHECK(kProtocolVersion >= 7);
    CHECK(net::kFirstResourceEntity == kMaxPlayers);
}

TEST_CASE("Interpolation : les ressources ne sont pas interpolées comme des joueurs") {
    client::Interpolation interpolation;
    net::EntityState walker{1, net::EntityKind::Player, {0, 0, 0}, 0, 0};
    interpolation.push(net::Snapshot{10, 0, {walker, resource(5, 0)}, {}, {}});
    walker.position[0] = 256;
    interpolation.push(net::Snapshot{11, 0, {walker, resource(5, 0)}, {}, {}});

    const auto shown = interpolation.sample();
    REQUIRE(shown.size() == 1);
    CHECK(shown[0].id == 1);
}
