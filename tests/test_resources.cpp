// Ressources récoltables côté serveur (T044) : règles de récolte, visée, placement déterministe,
// repousse et snapshots.
#include "GameWorld.hpp"
#include "resources/HarvestReach.hpp"
#include "resources/Harvesting.hpp"
#include "resources/ResourceScatter.hpp"

#include "odyssee/core/Config.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Terrain.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

using namespace odyssee;
using namespace odyssee::server;

namespace {

constexpr std::int32_t kMeter = 256;

constexpr std::string_view kItems = R"([item wood_log]
name = Bûche
stack = 20
category = resource

[item stone]
name = Pierre
stack = 40
category = resource

[item stone_axe]
name = Hache de pierre
stack = 1
category = tool
)";

/// Arbre : 3 coups, 10 ticks avec la hache (20 à la main), repousse en 100 ticks. Rocher : sans
/// outil, 2 coups de 5 ticks. Filon : seulement avec la hache.
constexpr std::string_view kResources = R"([resource tree]
name = Arbre
shape = tree
item = wood_log 1
hits = 3
hit_ticks = 10
tool = stone_axe
regrow = 100
abundance = 3

[resource rock]
name = Rocher
shape = rock
item = stone 2
hits = 2
hit_ticks = 5
regrow = 40
abundance = 1

[resource vein]
name = Filon
shape = vein
item = stone 1
hits = 1
hit_ticks = 5
tool = stone_axe
hand = no
regrow = 40
abundance = 0
)";

const data::Catalog& items() {
    static const data::Catalog catalog = *data::Catalog::fromText(kItems, "").catalog;
    return catalog;
}

data::ResourceCatalog resourceTypes() {
    auto result = data::ResourceCatalog::fromText(kResources, items());
    REQUIRE_MESSAGE(result.catalog.has_value(), (result.error ? result.error->describe() : ""));
    return std::move(*result.catalog);
}

const data::ResourceDef& typeNamed(const data::ResourceCatalog& types, std::string_view key) {
    const data::ResourceDef* type = types.find(key);
    REQUIRE(type != nullptr);
    return *type;
}

data::ItemId axe() {
    return *items().findItem("stone_axe");
}

constexpr data::ResourceTypeId kTree{0};
constexpr data::ResourceTypeId kRock{1};

/// Pieds du joueur 0 dans le monde sans carte (point d'apparition décalé de -3 m vers l'ouest).
std::array<std::int32_t, 3> playerZeroFeet(const GameWorld& world) {
    return world.motionOf(0).value().position;
}

std::array<std::int32_t, 3> offset(std::array<std::int32_t, 3> at, std::int32_t dx,
                                   std::int32_t dy) {
    at[0] += dx;
    at[1] += dy;
    return at;
}

/// Un tick du joueur 0 : il regarde au nord (cap 270°), immobile, clic gauche tenu ou non.
void tickHolding(GameWorld& world, net::Tick tick, bool primary = true) {
    net::InputCommand command;
    command.yaw = net::quantizeYaw(270.0f);
    command.buttons = primary ? static_cast<std::uint16_t>(net::InputButton::Primary) : 0;
    world.receiveInputs(0, net::InputBatch{tick, {command}});
    world.step();
}

/// Un monde sans carte, le joueur 0 connecté, un arbre 1,5 m devant lui (au nord).
GameWorld worldWithTreeAhead() {
    GameWorld world;
    const std::vector<ConnectedPlayer> one{{0, 1}};
    world.syncPlayers(one);
    const std::vector<PlacedResource> tree{
        {kTree, offset(playerZeroFeet(world), 0, -3 * kMeter / 2), 0}};
    CHECK(world.addResources(resourceTypes(), tree) == 1);
    return world;
}

std::uint16_t hitsLeftOf(const GameWorld& world, net::EntityId id) {
    return world.resourceNode(id).value().hitsLeft;
}

/// Relief plat à 10 m de `quads` × `quads` cases d'un mètre, en un seul bloc.
world::HeightField flatRelief(std::uint32_t quads) {
    world::HeightField field;
    field.quadsX = quads;
    field.quadsY = quads;
    field.chunkQuads = static_cast<std::uint16_t>(quads);
    field.chunksX = 1;
    field.chunksY = 1;
    field.samples.assign(std::size_t{quads + 1} * (quads + 1), world::rawFromHeight(10 * kMeter));
    return field;
}

/// Petit monde de 40 m × 40 m : relief à 10 m, un trou de 8 × 8 cases (x et y de 10 à 18 m), un
/// mur (x de 24 à 25 m, sur toute la largeur en y de 4 à 36 m) et un bâtiment à toit plat sur le
/// relief (x et y de 26 à 34 m, toit à 13,5 m).
world::WorldFile smallWorld() {
    world::WorldFile file;
    file.terrain = flatRelief(40);
    for (std::uint32_t row = 10; row < 18; ++row) {
        for (std::uint32_t col = 10; col < 18; ++col) {
            file.holes.push_back({col, row});
        }
    }
    file.boxes = {
        {{24 * kMeter + 128, 20 * kMeter, 11 * kMeter}, {128, 16 * kMeter, 1 * kMeter}, 0, {}},
        {{30 * kMeter, 30 * kMeter, 11 * kMeter + 128},
         {4 * kMeter, 4 * kMeter, 2 * kMeter},
         0,
         {}},
    };
    return file;
}

constexpr ScatterSettings kSmallScatter{0x5EED, 80, 0, 19 * kMeter, kMeter, 30};

bool isOnHole(const world::WorldFile& file, const PlacedResource& resource) {
    return file.isHole({static_cast<std::uint32_t>(resource.position[0] / kMeter),
                        static_cast<std::uint32_t>(resource.position[1] / kMeter)});
}

bool isInsideBox(const world::CollisionBox& box, const PlacedResource& resource) {
    const auto inside = [&](std::size_t axis) {
        const std::int64_t gap = std::int64_t{resource.position[axis]} - box.center[axis];
        return std::llabs(gap) <= std::int64_t{box.halfExtents[axis]};
    };
    return inside(0) && inside(1);
}

} // namespace

// --- Règles ---------------------------------------------------------------------------------

TEST_CASE("Récolte : cadence selon l'outil, à la main plus lent ou impossible") {
    const data::ResourceCatalog types = resourceTypes();
    const auto& tree = typeNamed(types, "tree");
    const auto& rock = typeNamed(types, "rock");
    const auto& vein = typeNamed(types, "vein");

    CHECK(hitInterval(tree, axe()) == 10u);
    CHECK(hitInterval(tree, kBareHands) == 10u * data::kBareHandSlowdown);
    CHECK(hitInterval(tree, *items().findItem("stone")) == 20u); // pas le bon outil : la main
    CHECK(hitInterval(rock, kBareHands) == 5u); // aucun outil demandé : la main suffit
    CHECK(hitInterval(rock, axe()) == 5u);
    CHECK(hitInterval(vein, axe()) == 5u);
    CHECK_FALSE(hitInterval(vein, kBareHands).has_value());
}

TEST_CASE("Récolte : chaque coup rapporte, le dernier épuise, la repousse remplit au bon tick") {
    const data::ResourceCatalog types = resourceTypes();
    const auto& tree = typeNamed(types, "tree");
    ResourceNode node = freshNode(tree);
    CHECK(node.hitsLeft == 3);

    for (int hit = 0; hit < 3; ++hit) {
        const auto harvest = tryHarvest(node, tree, kBareHands);
        REQUIRE(harvest.has_value());
        CHECK(*harvest == Harvest{*items().findItem("wood_log"), 1});
    }
    CHECK(node.depleted());
    CHECK_FALSE(tryHarvest(node, tree, axe()).has_value());

    for (std::uint32_t tick = 1; tick < tree.regrowTicks; ++tick) {
        stepRegrowth(node, tree);
    }
    CHECK(node.depleted());
    stepRegrowth(node, tree);
    CHECK(node.hitsLeft == 3);

    ResourceNode vein = freshNode(typeNamed(types, "vein"));
    CHECK_FALSE(tryHarvest(vein, typeNamed(types, "vein"), kBareHands).has_value());
    CHECK(vein.hitsLeft == 1); // coup refusé : rien n'est retiré
}

TEST_CASE("Visée : à portée et devant soi seulement") {
    const std::array<std::int32_t, 3> feet{100 * kMeter, 100 * kMeter, 10 * kMeter};
    const std::uint16_t north = net::quantizeYaw(270.0f); // vers -y
    const auto at = [&](std::int32_t dx, std::int32_t dy, std::int32_t dz = 0) {
        return std::array<std::int32_t, 3>{feet[0] + dx, feet[1] + dy, feet[2] + dz};
    };

    CHECK(isWithinReach(feet, north, at(0, -2 * kMeter)));
    CHECK_FALSE(isWithinReach(feet, north, at(0, -3 * kMeter)));         // trop loin
    CHECK_FALSE(isWithinReach(feet, north, at(0, 2 * kMeter)));          // dans le dos
    CHECK_FALSE(isWithinReach(feet, north, at(2 * kMeter, 0)));          // sur le côté (90°)
    CHECK(isWithinReach(feet, north, at(kMeter, -2 * kMeter)));          // 27° : dans le cône
    CHECK_FALSE(isWithinReach(feet, north, at(2 * kMeter, -kMeter)));    // 63° : hors du cône
    CHECK(isWithinReach(feet, north, at(0, kMeter / 4)));                // collé : peu importe
    CHECK_FALSE(isWithinReach(feet, north, at(0, -kMeter, 3 * kMeter))); // trop haut
}

// --- Placement --------------------------------------------------------------------------------

TEST_CASE("Placement : même graine, mêmes ressources ; autre graine, autres ressources") {
    const data::ResourceCatalog types = resourceTypes();
    const world::WorldTerrain terrain(smallWorld());
    const std::array<std::int32_t, 3> center{20 * kMeter, 20 * kMeter, 0};

    const auto first = scatterResources(terrain, center, types.types(), kSmallScatter);
    const auto again = scatterResources(terrain, center, types.types(), kSmallScatter);
    CHECK(first.size() > 20);
    CHECK(first == again);

    ScatterSettings other = kSmallScatter;
    other.seed = 0xBEEF;
    CHECK(scatterResources(terrain, center, types.types(), other) != first);
}

TEST_CASE("Placement : sur le relief, ni dans un trou, ni dans un mur, ni sur un toit") {
    const world::WorldFile file = smallWorld();
    const data::ResourceCatalog types = resourceTypes();
    const world::WorldTerrain terrain(file);
    const auto placed =
        scatterResources(terrain, {20 * kMeter, 20 * kMeter, 0}, types.types(), kSmallScatter);
    REQUIRE_FALSE(placed.empty());

    for (const PlacedResource& resource : placed) {
        CHECK(resource.position[2] == 10 * kMeter); // posée sur le relief
        CHECK_FALSE(isOnHole(file, resource));
        CHECK_FALSE(isInsideBox(file.boxes[0], resource));
        CHECK_FALSE(isInsideBox(file.boxes[1], resource));
        CHECK(resource.type != data::ResourceTypeId{2}); // abondance 0 : jamais placée
        for (const PlacedResource& other : placed) {
            if (&other != &resource) {
                CHECK(horizontalDistanceSquared(resource.position, other.position) >=
                      std::int64_t{kMeter} * kMeter);
            }
        }
    }
}

TEST_CASE("Placement : dans l'anneau demandé, sur le sol plat provisoire aussi") {
    const data::ResourceCatalog types = resourceTypes();
    const sim::FlatTerrain flat(5 * kMeter);
    const std::array<std::int32_t, 3> center{1000 * kMeter, 1000 * kMeter, 5 * kMeter};
    const auto placed = scatterResources(flat, center, types.types(), kProvisionalScatter);
    CHECK(placed.size() == kProvisionalScatter.count);
    for (const PlacedResource& resource : placed) {
        const std::int64_t distance = horizontalDistanceSquared(center, resource.position);
        CHECK(distance >=
              std::int64_t{kProvisionalScatter.minRadius} * kProvisionalScatter.minRadius);
        CHECK(distance <=
              std::int64_t{kProvisionalScatter.maxRadius} * kProvisionalScatter.maxRadius);
        CHECK(resource.position[2] == 5 * kMeter);
    }
}

// --- Dans le monde ------------------------------------------------------------------------------

TEST_CASE("Monde : clic gauche tenu devant un arbre, un coup toutes les 20 ticks à la main") {
    GameWorld world = worldWithTreeAhead();
    const net::EntityId tree = net::kFirstResourceEntity;
    REQUIRE(world.resourceNode(tree).has_value());
    CHECK_FALSE(world.resourceNode(static_cast<net::EntityId>(tree + 1)).has_value());
    CHECK_FALSE(world.resourceNode(0).has_value());

    tickHolding(world, 1);
    CHECK(hitsLeftOf(world, tree) == 2); // premier coup tout de suite
    for (net::Tick tick = 2; tick <= 20; ++tick) {
        tickHolding(world, tick);
    }
    CHECK(hitsLeftOf(world, tree) == 2); // cadence : 20 ticks à mains nues
    tickHolding(world, 21);
    CHECK(hitsLeftOf(world, tree) == 1);
}

TEST_CASE("Monde : sans clic, ou en regardant ailleurs, rien n'est récolté") {
    GameWorld world = worldWithTreeAhead();
    for (net::Tick tick = 1; tick <= 30; ++tick) {
        tickHolding(world, tick, false);
    }
    CHECK(hitsLeftOf(world, net::kFirstResourceEntity) == 3);

    net::InputCommand south;
    south.yaw = net::quantizeYaw(90.0f);
    south.buttons = static_cast<std::uint16_t>(net::InputButton::Primary);
    for (net::Tick tick = 31; tick <= 60; ++tick) {
        world.receiveInputs(0, net::InputBatch{tick, {south}});
        world.step();
    }
    CHECK(hitsLeftOf(world, net::kFirstResourceEntity) == 3);
}

TEST_CASE(
    "Monde : épuisé au dernier coup, plein de nouveau au bon tick, visible dans le snapshot") {
    GameWorld world = worldWithTreeAhead();
    const net::EntityId tree = net::kFirstResourceEntity;
    net::Tick tick = 1;
    for (; tick <= 41; ++tick) { // coups aux ticks 1, 21 et 41
        tickHolding(world, tick);
    }
    REQUIRE(world.resourceNode(tree)->depleted());
    const auto depletedState = world.snapshotFor(0, tick);
    REQUIRE(depletedState.entities.size() == 2);
    CHECK(depletedState.entities[1].kind == net::EntityKind::Resource);
    CHECK(depletedState.entities[1].flags == (kTree.value | net::kResourceDepleted));

    // Repousse en 100 ticks après le coup du tick 41 : pleine au tick 141, pas avant.
    for (; tick <= 140; ++tick) {
        tickHolding(world, tick, false);
    }
    CHECK(world.resourceNode(tree)->depleted());
    tickHolding(world, tick, false);
    CHECK(hitsLeftOf(world, tree) == 3);
    CHECK(world.snapshotFor(0, tick).entities[1].flags == kTree.value);
}

TEST_CASE("Snapshot : les joueurs d'abord, puis les ressources proches, au plus la limite") {
    GameWorld world;
    const std::vector<ConnectedPlayer> two{{0, 1}, {3, 2}};
    world.syncPlayers(two);
    const auto feet = playerZeroFeet(world);

    std::vector<PlacedResource> placed;
    placed.push_back({kRock, offset(feet, 0, -59 * kMeter), 0}); // juste dans le rayon
    placed.push_back({kRock, offset(feet, 0, -61 * kMeter), 0}); // juste au-delà
    placed.push_back({kTree, offset(feet, 5 * kMeter, 0), 0});   // la plus proche
    for (std::int32_t i = 0; i < 100; ++i) {                     // de quoi dépasser la limite
        placed.push_back({kTree, offset(feet, 10 * kMeter + i * 40, 10 * kMeter), 0});
    }
    world.addResources(resourceTypes(), placed);

    const net::Snapshot snapshot = world.snapshotFor(0, 1);
    REQUIRE(snapshot.entities.size() == kMaxSnapshotEntities);
    CHECK(snapshot.entities[0].kind == net::EntityKind::Player);
    CHECK(snapshot.entities[1].kind == net::EntityKind::Player);
    CHECK(snapshot.entities[2].id == net::kFirstResourceEntity + 2); // la plus proche d'abord
    std::int64_t previous = 0;
    for (std::size_t i = 2; i < snapshot.entities.size(); ++i) {
        const net::EntityState& entity = snapshot.entities[i];
        CHECK(entity.kind == net::EntityKind::Resource);
        CHECK(entity.id >= net::kFirstResourceEntity);
        const std::int64_t distance = horizontalDistanceSquared(feet, entity.position);
        CHECK(distance >= previous);
        previous = distance;
    }

    // Peu de ressources : celle au-delà de 60 m ne part pas, celle juste en deçà si.
    GameWorld sparse;
    sparse.syncPlayers(two);
    sparse.addResources(resourceTypes(), std::span(placed).first(2));
    const net::Snapshot near = sparse.snapshotFor(0, 1);
    REQUIRE(near.entities.size() == 3);
    CHECK(near.entities[2].id == net::kFirstResourceEntity);
}

TEST_CASE("Snapshot : un joueur absent du monde ne reçoit pas de ressources") {
    GameWorld world;
    const std::vector<PlacedResource> one{{kTree, {0, 0, 0}, 0}};
    world.addResources(resourceTypes(), one);
    CHECK(world.snapshotFor(2, 1).entities.empty());
}

TEST_CASE("Monde : le bois récolté arrive dans l'inventaire du joueur (T045)") {
    GameWorld world = worldWithTreeAhead();
    world.useCatalog(items());
    (void)world.takeInventoryUpdate(0); // inventaire vide de l'apparition

    tickHolding(world, 1); // premier coup tout de suite
    const auto woodLog = items().findItem("wood_log");
    REQUIRE(woodLog);
    REQUIRE(world.inventoryOf(0) != nullptr);
    CHECK(server::countItem(*world.inventoryOf(0), *woodLog) > 0);
    CHECK(world.takeInventoryUpdate(0)); // le joueur en est prévenu
}

TEST_CASE("Monde : la hache tenue (case active) récolte deux fois plus vite (T047)") {
    GameWorld world = worldWithTreeAhead();
    world.useCatalog(items());
    const auto axe = items().findItem("stone_axe");
    REQUIRE(axe);
    CHECK(world.giveItem(0, *axe, 1) == 0); // case 0, tenue par défaut
    const net::EntityId tree = net::kFirstResourceEntity;

    tickHolding(world, 1); // premier coup tout de suite
    CHECK(hitsLeftOf(world, tree) == 2);
    for (net::Tick tick = 2; tick <= 10; ++tick) {
        tickHolding(world, tick);
    }
    CHECK(hitsLeftOf(world, tree) == 2);
    tickHolding(world, 11); // avec l'outil : un coup tous les 10 ticks (20 à mains nues)
    CHECK(hitsLeftOf(world, tree) == 1);
}
