#include "odyssee/sim/Movement.hpp"

#include "odyssee/net/Quantize.hpp"
#include "odyssee/sim/FixedMath.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace odyssee::sim {

namespace {

struct Vec2 {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

std::int32_t length(std::int64_t x, std::int64_t y) {
    return static_cast<std::int32_t>(isqrt(static_cast<std::uint64_t>(x * x + y * y)));
}

std::int16_t toInt16(std::int32_t value) {
    constexpr std::int32_t low = std::numeric_limits<std::int16_t>::min();
    constexpr std::int32_t high = std::numeric_limits<std::int16_t>::max();
    return static_cast<std::int16_t>(std::clamp(value, low, high));
}

/// Position après un déplacement, ramenée dans les bornes du monde (calcul sur 64 bits : la
/// somme elle-même ne peut pas déborder).
std::int32_t advance(std::int32_t position, std::int32_t delta) {
    const std::int64_t moved = std::int64_t{position} + delta;
    return static_cast<std::int32_t>(
        std::clamp(moved, -std::int64_t{kMaxCoordinate}, std::int64_t{kMaxCoordinate}));
}

/// Direction voulue, en Q15 (longueur 1 au plus) : avancer selon le cap, pas de côté à droite.
/// Avant = (cos, sin) du cap ; droite = (-sin, cos) (x est, y sud : comme Unreal, X puis Y).
/// En diagonale, la longueur est ramenée à 1 : on ne va pas plus vite en crabe.
Vec2 wishDirection(const net::InputCommand& input) {
    const std::int64_t c = cosQ15(input.yaw);
    const std::int64_t s = sinQ15(input.yaw);
    const std::int64_t forward = input.moveForward;
    const std::int64_t right = input.moveRight;
    const std::int64_t x = forward * c - right * s; // unités : Q15 × 127
    const std::int64_t y = forward * s + right * c;

    const std::int64_t norm = length(x, y);
    if (norm == 0) {
        return {};
    }
    const std::int64_t divisor = std::max(norm, std::int64_t{net::kMaxAxisUnits} * kQ15One);
    return {static_cast<std::int32_t>(x * kQ15One / divisor),
            static_cast<std::int32_t>(y * kQ15One / divisor)};
}

std::int32_t targetSpeed(bool crouched, bool sprinting, const MovementTuning& tuning) {
    if (crouched) {
        return tuning.crouchSpeed;
    }
    return sprinting ? tuning.sprintSpeed : tuning.walkSpeed;
}

/// Rapproche la vitesse horizontale de la vitesse voulue, d'au plus `accel` par tick.
void accelerate(PlayerState& state, Vec2 wish, std::int32_t speed, std::int32_t accel) {
    const std::int32_t targetX = wish.x * speed / kQ15One;
    const std::int32_t targetY = wish.y * speed / kQ15One;
    const std::int32_t dx = targetX - state.velocity[0];
    const std::int32_t dy = targetY - state.velocity[1];
    const std::int32_t gap = length(dx, dy);
    if (gap <= accel) {
        state.velocity[0] = toInt16(targetX);
        state.velocity[1] = toInt16(targetY);
        return;
    }
    state.velocity[0] = toInt16(state.velocity[0] + dx * accel / gap);
    state.velocity[1] = toInt16(state.velocity[1] + dy * accel / gap);
}

/// Saute si on est au sol, qu'on le demande et qu'il reste assez d'endurance.
bool tryJump(PlayerState& state, const net::InputCommand& input, bool grounded,
             const MovementTuning& tuning) {
    if (!grounded || !input.isPressed(net::InputButton::Jump) ||
        state.stamina < tuning.jumpCost) {
        return false;
    }
    state.velocity[2] = toInt16(tuning.jumpSpeed);
    state.stamina = static_cast<std::uint16_t>(state.stamina - tuning.jumpCost);
    return true;
}

void applyGravity(PlayerState& state, bool onGround, const MovementTuning& tuning) {
    if (onGround) {
        state.velocity[2] = 0;
        return;
    }
    state.velocity[2] = toInt16(std::max(state.velocity[2] - tuning.gravity, -tuning.maxFallSpeed));
}

/// Déplacement horizontal. Au sol, un mur ou une pente de plus de 45° (montée plus grande que la
/// distance parcourue, au-delà d'une petite marche) bloque le joueur.
void moveHorizontally(PlayerState& state, const Terrain& terrain, bool grounded,
                      const MovementTuning& tuning) {
    const std::int32_t x = advance(state.position[0], state.velocity[0]);
    const std::int32_t y = advance(state.position[1], state.velocity[1]);
    if (grounded) {
        // Sur 64 bits : la hauteur du sol vient de la carte, la différence ne doit pas déborder.
        const std::int64_t rise = std::int64_t{terrain.groundHeight(x, y)} - state.position[2];
        const std::int64_t distance = length(state.velocity[0], state.velocity[1]);
        if (rise > distance + tuning.stepHeight) {
            state.velocity[0] = 0;
            state.velocity[1] = 0;
            return;
        }
    }
    state.position[0] = x;
    state.position[1] = y;
}

/// Déplacement vertical et contact avec le sol. En descente, un joueur qui était au sol y reste
/// collé (pas de petits sauts à chaque tick sur une pente). Renvoie vrai s'il est au sol.
bool moveVertically(PlayerState& state, const Terrain& terrain, bool stickToGround,
                    const MovementTuning& tuning) {
    state.position[2] = advance(state.position[2], state.velocity[2]);
    const std::int32_t ground = terrain.groundHeight(state.position[0], state.position[1]);
    const bool landed = state.position[2] <= ground;
    const bool snapped = stickToGround && state.velocity[2] <= 0 &&
                         std::int64_t{state.position[2]} - ground <= tuning.snapDown;
    if (landed || snapped) {
        state.position[2] = ground;
        state.velocity[2] = 0;
        return true;
    }
    return false;
}

void updateStamina(PlayerState& state, bool sprinted, bool moving,
                   const MovementTuning& tuning) {
    std::int32_t stamina = state.stamina;
    if (sprinted) {
        stamina -= std::min(tuning.sprintCost, stamina);
    } else {
        stamina += moving ? tuning.regenMoving : tuning.regenStill;
    }
    state.stamina = static_cast<std::uint16_t>(std::clamp(stamina, 0, tuning.maxStamina));
}

/// Vidée en courant, l'endurance doit remonter jusqu'à `exhaustedUntil` avant de pouvoir recourir :
/// sinon on alternerait course et marche à chaque tick, endurance à zéro.
bool isExhausted(const PlayerState& before, const PlayerState& after,
                 const MovementTuning& tuning) {
    if (after.stamina == 0) {
        return true;
    }
    return before.has(kExhausted) && after.stamina < tuning.exhaustedUntil;
}

std::uint8_t flagsOf(bool grounded, bool crouched, bool sprinting, bool exhausted) {
    unsigned flags = 0;
    flags |= grounded ? unsigned{kGrounded} : 0U;
    flags |= crouched ? unsigned{kCrouched} : 0U;
    flags |= sprinting ? unsigned{kSprinting} : 0U;
    flags |= exhausted ? unsigned{kExhausted} : 0U;
    return static_cast<std::uint8_t>(flags);
}

} // namespace

PlayerState step(const PlayerState& state, const net::InputCommand& input, const Terrain& terrain,
                 const MovementTuning& tuning) {
    PlayerState next = state;
    next.yaw = input.yaw;
    next.pitch = input.pitch;

    const bool grounded = state.has(kGrounded);
    const bool crouched = input.isPressed(net::InputButton::Crouch);
    const Vec2 wish = wishDirection(input);
    const bool moving = wish.x != 0 || wish.y != 0;
    const bool sprinting = input.isPressed(net::InputButton::Sprint) && !crouched && moving &&
                           !state.has(kExhausted) && state.stamina > 0;

    accelerate(next, wish, targetSpeed(crouched, sprinting, tuning),
               grounded ? tuning.groundAccel : tuning.airAccel);
    const bool jumped = tryJump(next, input, grounded, tuning);
    applyGravity(next, grounded && !jumped, tuning);
    moveHorizontally(next, terrain, grounded && !jumped, tuning);
    const bool nowGrounded = moveVertically(next, terrain, grounded && !jumped, tuning);

    if (!jumped) {
        updateStamina(next, sprinting && grounded, moving, tuning);
    }
    next.flags = flagsOf(nowGrounded, crouched, sprinting, isExhausted(state, next, tuning));
    return next;
}

} // namespace odyssee::sim
