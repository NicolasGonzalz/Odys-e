#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Movement.hpp"
#include "odyssee/world/CollisionShape.hpp"
#include "odyssee/world/WorldFileLoader.hpp"
#include "odyssee/world/WorldTerrain.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdint>

using namespace odyssee;
using namespace odyssee::world;

namespace {

/// 1 m en unités de simulation.
constexpr std::int32_t kMeter = 256;

/// Relief de `quads` × `quads` cases d'un mètre, en un seul bloc ; hauteur du point (col, row)
/// donnée par `height` (en 1/256 m, multiple de 4).
template <typename Height> HeightField relief(std::uint32_t quads, Height height) {
    HeightField field;
    field.quadsX = quads;
    field.quadsY = quads;
    field.chunkQuads = static_cast<std::uint16_t>(quads);
    field.chunksX = 1;
    field.chunksY = 1;
    for (std::uint32_t row = 0; row <= quads; ++row) {
        for (std::uint32_t col = 0; col <= quads; ++col) {
            field.samples.push_back(rawFromHeight(height(col, row)));
        }
    }
    return field;
}

CollisionBox box(std::array<std::int32_t, 3> center, std::array<std::uint32_t, 3> half,
                 std::uint16_t yaw = 0, BoxShape shape = BoxShape::Box) {
    return CollisionBox{center, half, yaw, shape};
}

/// Une petite carte de 20 m × 20 m, relief plat à 10 m. Sous le relief, un « bunker » : un sol
/// dont le dessus est à 2 m (x de 2 à 18 m, y de 2 à 10 m), sous 4 × 4 cases percées (x et y
/// de 4 à 8 m) qui servent d'entrée. Un mur de 3 m de haut posé sur ce sol, à x = 12 m. Un plafond
/// juste au-dessus de la tête du joueur, de x = 14 à 18 m.
WorldFile bunkerWorld() {
    WorldFile world;
    world.terrain = relief(20, [](auto, auto) { return 10 * kMeter; });
    for (std::uint32_t row = 4; row < 8; ++row) {
        for (std::uint32_t col = 4; col < 8; ++col) {
            world.holes.push_back({col, row});
        }
    }
    world.boxes = {
        box({10 * kMeter, 6 * kMeter, 1 * kMeter}, {8 * kMeter, 4 * kMeter, 1 * kMeter}),
        box({12 * kMeter, 6 * kMeter, 2 * kMeter + 384}, {kMeter / 4, 4 * kMeter, 384}),
        box({16 * kMeter, 6 * kMeter, 4 * kMeter + kMeter / 2},
            {2 * kMeter, 4 * kMeter, kMeter / 4}),
    };
    world.markers = {{MarkerKind::Place, {1, 2, 3}, 0, "ferme"},
                     {MarkerKind::PlayerSpawn, {6 * kMeter, 6 * kMeter, 2 * kMeter}, 0, "stase"}};
    return world;
}

sim::PlayerState standingAt(std::int32_t x, std::int32_t y, std::int32_t z) {
    sim::PlayerState state;
    state.position = {x, y, z};
    return state;
}

net::InputCommand forwardEast(std::uint16_t buttons = 0) {
    return net::InputCommand{net::quantizeAxis(1.0f), 0, net::quantizeYaw(0.0f), 0, buttons, 0};
}

sim::PlayerState run(sim::PlayerState state, const net::InputCommand& command, int ticks,
                     const sim::Terrain& terrain) {
    for (int i = 0; i < ticks; ++i) {
        state = sim::step(state, command, terrain);
    }
    return state;
}

} // namespace

TEST_CASE("Relief : la surface passe par les points et s'interpole sur les deux triangles") {
    // Une seule case : (0,0) à 0, (1,0) à 1 m, (0,1) à 2 m, (1,1) à 3 m.
    const HeightField field = relief(1, [](std::uint32_t col, std::uint32_t row) {
        return static_cast<std::int32_t>(col + 2 * row) * kMeter;
    });

    CHECK(field.surfaceHeight(0, 0) == 0);
    CHECK(field.surfaceHeight(kMeter, 0) == kMeter);
    CHECK(field.surfaceHeight(0, kMeter) == 2 * kMeter);
    CHECK(field.surfaceHeight(kMeter, kMeter) == 3 * kMeter);
    // Sur la diagonale, au milieu : moyenne de (0,0) et (1,1), quel que soit le triangle.
    CHECK(field.surfaceHeight(128, 128) == 384);
    // Triangle du haut (x ≥ y) : (0,0) (1,0) (1,1). En (1, 0,5) : entre 1 m et 3 m.
    CHECK(field.surfaceHeight(kMeter, 128) == 2 * kMeter);
    // Triangle du bas : (0,0) (0,1) (1,1). En (0, 0,5) : entre 0 et 2 m.
    CHECK(field.surfaceHeight(0, 128) == kMeter);
    // Hors de la carte : la hauteur du bord.
    CHECK(field.surfaceHeight(-500, -500) == 0);
    CHECK(field.surfaceHeight(9000, 9000) == 3 * kMeter);
}

TEST_CASE("Relief : une pente descendante s'arrondit comme une montante (vers le bas)") {
    const HeightField down = relief(
        1, [](std::uint32_t col, std::uint32_t) { return -static_cast<std::int32_t>(col) * 4; });
    // Pente de -4 sur 256 : en x = 1, -4/256 arrondi vers le bas = -1.
    CHECK(down.surfaceHeight(1, 0) == -1);
}

TEST_CASE("Volume : pavé tourné, cylindre, élargissement") {
    const CollisionBox plain = box({0, 0, 0}, {100, 10, 50});
    CHECK(covers(plain, 100, 10, 0));
    CHECK_FALSE(covers(plain, 101, 0, 0));
    CHECK(covers(plain, 101, 0, 1));

    // Tourné d'un quart de tour : la longueur passe sur l'axe y.
    const CollisionBox turned = box({0, 0, 0}, {100, 10, 50}, 16384);
    CHECK(covers(turned, 0, 95, 0));
    CHECK_FALSE(covers(turned, 95, 0, 0));

    // Cylindre : le coin du carré qui l'entoure est dehors, le bord du cercle dedans.
    const CollisionBox pillar = box({0, 0, 0}, {100, 100, 50}, 0, BoxShape::Cylinder);
    CHECK(covers(pillar, 100, 0, 0));
    CHECK(covers(pillar, 70, 70, 0));
    CHECK_FALSE(covers(pillar, 80, 80, 0));

    CHECK(bottomOf(plain) == -50);
    CHECK(topOf(plain) == 50);
}

TEST_CASE("WorldTerrain : le sol dépend de la hauteur, pas seulement de x et y") {
    const WorldTerrain terrain(bunkerWorld());
    const std::int32_t x = 10 * kMeter;
    const std::int32_t y = 6 * kMeter;

    // Au-dessus du relief : le relief. Dans le bunker (sous le relief) : le sol du bunker.
    CHECK(terrain.groundHeight(x, y) == 10 * kMeter);
    CHECK(terrain.groundBelow(x, y, 3 * kMeter) == 2 * kMeter);
    // Sous tout : rien.
    CHECK(terrain.groundBelow(x, y, -kMeter) == sim::kNoGround);
    // La case percée n'a pas de relief : on y voit le sol du bunker, de haut.
    CHECK(terrain.groundHeight(4 * kMeter + 128, 4 * kMeter + 128) == 2 * kMeter);
}

TEST_CASE("WorldTerrain : murs, plafond et bords de la carte") {
    const WorldTerrain terrain(bunkerWorld());
    const std::int32_t y = 6 * kMeter;
    const std::int32_t feet = 2 * kMeter;

    CHECK(terrain.isSolid(12 * kMeter, y, feet + 64, feet + 461));
    // Juste devant le mur, à moins d'un rayon de joueur : bloqué aussi.
    CHECK(terrain.isSolid(12 * kMeter - kMeter / 4 - WorldTerrain::kPlayerRadius + 1, y, feet + 64,
                          feet + 461));
    CHECK_FALSE(terrain.isSolid(10 * kMeter, y, feet + 64, feet + 461));
    // Au-dessus du mur (3 m) : libre.
    CHECK_FALSE(terrain.isSolid(12 * kMeter, y, 6 * kMeter, 7 * kMeter));

    CHECK(terrain.isSolid(-1, y, 0, 10));
    CHECK(terrain.isSolid(21 * kMeter, y, 0, 10));
}

TEST_CASE("WorldTerrain : le joueur du bunker bute sur le mur, sans le traverser") {
    const WorldTerrain terrain(bunkerWorld());
    const auto start = standingAt(8 * kMeter, 6 * kMeter, 2 * kMeter);

    const auto after = run(start, forwardEast(), 60, terrain); // 3 s de marche vers l'est
    CHECK(after.position[0] < 12 * kMeter - kMeter / 4 - WorldTerrain::kPlayerRadius);
    CHECK(after.position[0] > 10 * kMeter);
    CHECK(after.position[2] == 2 * kMeter); // toujours sur le sol du bunker, pas sur le relief
}

TEST_CASE("WorldTerrain : un saut sous un plafond s'y cogne et retombe") {
    const WorldTerrain terrain(bunkerWorld());
    const std::int32_t floor = 2 * kMeter;
    const std::int32_t ceiling = 4 * kMeter + kMeter / 4; // dessous du plafond
    auto state = standingAt(16 * kMeter, 6 * kMeter, floor);

    state = sim::step(
        state, net::InputCommand{0, 0, 0, 0, static_cast<std::uint16_t>(net::InputButton::Jump), 0},
        terrain);
    std::int32_t highest = state.position[2];
    for (int i = 0; i < 40; ++i) {
        state = sim::step(state, net::InputCommand{}, terrain);
        highest = std::max(highest, state.position[2]);
    }
    CHECK(highest + sim::kDefaultTuning.height <= ceiling);
    CHECK(state.position[2] == floor);
}

TEST_CASE("WorldTerrain : par la case percée, le joueur tombe dans le bunker") {
    const WorldTerrain terrain(bunkerWorld());
    // Debout sur le relief, juste à l'ouest de l'entrée, il avance vers l'est. (Un trou d'un
    // seul mètre se franchirait en marchant : le bord est rattrapé comme une petite marche.)
    const auto start = standingAt(3 * kMeter + 128, 6 * kMeter, 10 * kMeter);

    const auto after = run(start, forwardEast(), 40, terrain);
    CHECK(after.position[2] == 2 * kMeter);
}

TEST_CASE("WorldTerrain : point d'apparition de la carte") {
    const auto spawn = findPlayerSpawn(bunkerWorld());
    REQUIRE(spawn);
    CHECK(spawn->id == "stase");
    CHECK_FALSE(findPlayerSpawn(WorldFile{}));
}

TEST_CASE("Chargement : un fichier absent est une erreur, pas un plantage") {
    const auto loaded = loadWorldFile("ce/fichier/n/existe/pas.world");
    CHECK_FALSE(loaded.world);
    CHECK(loaded.error == WorldFileError::CannotOpen);
}
