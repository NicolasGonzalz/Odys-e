#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/Movement.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <cstdlib>

using namespace odyssee;
using namespace odyssee::sim;

namespace {

constexpr MovementTuning kTune{};
constexpr std::int32_t kGround = 0;

/// 1 m en unités de simulation.
constexpr std::int32_t kMeter = 256;

PlayerState standing() {
    PlayerState state;
    state.position = {1000 * kMeter, 1000 * kMeter, kGround};
    return state;
}

net::InputCommand input(float forward, float right, float yawDegrees,
                        std::uint16_t buttons = 0) {
    return net::InputCommand{net::quantizeAxis(forward), net::quantizeAxis(right),
                             net::quantizeYaw(yawDegrees), 0, buttons};
}

constexpr auto button(net::InputButton b) { return static_cast<std::uint16_t>(b); }

PlayerState run(PlayerState state, const net::InputCommand& command, int ticks,
                const Terrain& terrain) {
    for (int i = 0; i < ticks; ++i) {
        state = step(state, command, terrain);
    }
    return state;
}

/// Une falaise : sol à `low` avant x = edge, à `high` après.
class Cliff final : public Terrain {
  public:
    Cliff(std::int32_t edge, std::int32_t low, std::int32_t high)
        : _edge(edge), _low(low), _high(high) {}
    [[nodiscard]] std::int32_t groundHeight(std::int32_t x, std::int32_t) const override {
        return x < _edge ? _low : _high;
    }

  private:
    std::int32_t _edge, _low, _high;
};

/// Une pente régulière vers l'est : le sol descend de 1 m tous les `run` mètres.
class Slope final : public Terrain {
  public:
    explicit Slope(std::int32_t run) : _run(run) {}
    [[nodiscard]] std::int32_t groundHeight(std::int32_t x, std::int32_t) const override {
        return -x / _run;
    }

  private:
    std::int32_t _run;
};

} // namespace

TEST_CASE("Mouvement : sans input, le joueur reste immobile au sol") {
    const FlatTerrain flat(kGround);
    const PlayerState after = run(standing(), input(0, 0, 0), 20, flat);
    CHECK(after.position == standing().position);
    CHECK(after.has(kGrounded));
    CHECK(after.stamina == kTune.maxStamina);
}

TEST_CASE("Mouvement : marcher dans la direction du cap, à la vitesse de marche") {
    const FlatTerrain flat(kGround);

    SUBCASE("cap 0° : vers l'est (+x)") {
        const PlayerState after = run(standing(), input(1, 0, 0), 10, flat);
        CHECK(after.velocity[0] == kTune.walkSpeed);
        CHECK(after.velocity[1] == 0);
        CHECK(after.position[0] > standing().position[0] + 8 * kTune.walkSpeed);
    }
    SUBCASE("cap 90° : vers le sud (+y)") {
        const PlayerState after = run(standing(), input(1, 0, 90), 10, flat);
        CHECK(std::abs(after.velocity[0]) <= 1);
        CHECK(after.velocity[1] == kTune.walkSpeed);
    }
    SUBCASE("pas de côté à droite, cap 0° : vers le sud (+y)") {
        const PlayerState after = run(standing(), input(0, 1, 0), 10, flat);
        CHECK(after.velocity[1] == kTune.walkSpeed);
    }
    SUBCASE("en diagonale, pas plus vite") {
        const PlayerState after = run(standing(), input(1, 1, 0), 10, flat);
        const auto speed2 = after.velocity[0] * after.velocity[0] +
                            after.velocity[1] * after.velocity[1];
        CHECK(speed2 <= kTune.walkSpeed * kTune.walkSpeed);
        CHECK(speed2 >= (kTune.walkSpeed - 2) * (kTune.walkSpeed - 2));
    }
}

TEST_CASE("Mouvement : la course consomme l'endurance, puis on marche") {
    const FlatTerrain flat(kGround);
    const auto sprint = input(1, 0, 0, button(net::InputButton::Sprint));

    PlayerState state = run(standing(), sprint, 10, flat);
    CHECK(state.velocity[0] == kTune.sprintSpeed);
    CHECK(state.has(kSprinting));
    CHECK(state.stamina < kTune.maxStamina);

    int ticks = 0;
    while (state.stamina > 0 && ticks < 1000) {
        state = step(state, sprint, flat);
        ++ticks;
    }
    CHECK(ticks > 140); // ~8 s de course
    CHECK(state.has(kExhausted));

    // Épuisé : on marche, même en gardant Maj, jusqu'à ce que l'endurance remonte assez.
    state = run(state, sprint, 10, flat);
    CHECK(state.velocity[0] == kTune.walkSpeed);
    CHECK_FALSE(state.has(kSprinting));
    CHECK(state.stamina == 10 * kTune.regenMoving);

    state = run(state, input(0, 0, 0), 40, flat); // à l'arrêt, elle remonte plus vite
    CHECK(state.stamina == 10 * kTune.regenMoving + 40 * kTune.regenStill);
    CHECK_FALSE(state.has(kExhausted));
    state = run(state, sprint, 5, flat);
    CHECK(state.has(kSprinting));
}

TEST_CASE("Mouvement : saut d'environ 1 m, puis retour au sol") {
    const FlatTerrain flat(kGround);
    const auto jump = input(0, 0, 0, button(net::InputButton::Jump));

    PlayerState state = step(standing(), jump, flat);
    CHECK_FALSE(state.has(kGrounded));
    CHECK(state.stamina == kTune.maxStamina - kTune.jumpCost);

    std::int32_t apex = state.position[2];
    int ticks = 1;
    while (!state.has(kGrounded) && ticks < 100) {
        state = step(state, input(0, 0, 0), flat); // en l'air, sauter ne fait rien
        apex = std::max(apex, state.position[2]);
        ++ticks;
    }
    CHECK(state.has(kGrounded));
    CHECK(state.position[2] == kGround);
    CHECK(apex > 85 * kMeter / 100);
    CHECK(apex < 115 * kMeter / 100);
    CHECK(ticks > 15); // ~0,9 s en l'air à 20 ticks/s
    CHECK(ticks < 25);
}

TEST_CASE("Mouvement : pas de saut sans endurance suffisante") {
    const FlatTerrain flat(kGround);
    PlayerState tired = standing();
    tired.stamina = static_cast<std::uint16_t>(kTune.jumpCost - 1);
    const PlayerState after = step(tired, input(0, 0, 0, button(net::InputButton::Jump)), flat);
    CHECK(after.has(kGrounded));
    CHECK(after.velocity[2] == 0);
}

TEST_CASE("Mouvement : accroupi, on avance lentement et on ne court pas") {
    const FlatTerrain flat(kGround);
    const auto crouchSprint = input(1, 0, 0, button(net::InputButton::Crouch) |
                                                  button(net::InputButton::Sprint));
    const PlayerState after = run(standing(), crouchSprint, 10, flat);
    CHECK(after.velocity[0] == kTune.crouchSpeed);
    CHECK(after.has(kCrouched));
    CHECK_FALSE(after.has(kSprinting));
    CHECK(after.stamina == kTune.maxStamina);
}

TEST_CASE("Mouvement : un mur bloque, une petite marche se franchit") {
    const std::int32_t edge = standing().position[0] + 5 * kMeter;

    const Cliff wall(edge, kGround, kGround + 2 * kMeter);
    const PlayerState blocked = run(standing(), input(1, 0, 0), 60, wall);
    CHECK(blocked.position[0] < edge);
    CHECK(blocked.position[2] == kGround);

    const Cliff step20cm(edge, kGround, kGround + 20 * kMeter / 100);
    const PlayerState climbed = run(standing(), input(1, 0, 0), 60, step20cm);
    CHECK(climbed.position[0] > edge);
    CHECK(climbed.position[2] == kGround + 20 * kMeter / 100);
    CHECK(climbed.has(kGrounded));
}

TEST_CASE("Mouvement : en descente, on reste collé au sol ; d'une falaise, on tombe") {
    const Slope gentle(4); // 1 m de descente tous les 4 m (14°)
    PlayerState state = standing();
    state.position[2] = gentle.groundHeight(state.position[0], state.position[1]);
    for (int i = 0; i < 40; ++i) {
        state = step(state, input(1, 0, 0), gentle);
        CHECK(state.has(kGrounded));
    }

    const std::int32_t edge = standing().position[0] + kMeter;
    const Cliff drop(edge, kGround, kGround - 5 * kMeter);
    state = run(standing(), input(1, 0, 0), 8, drop); // ~3,3 m : au-delà du bord
    CHECK_FALSE(state.has(kGrounded)); // dans le vide
    state = run(state, input(1, 0, 0), 40, drop);
    CHECK(state.has(kGrounded));
    CHECK(state.position[2] == kGround - 5 * kMeter);
}
