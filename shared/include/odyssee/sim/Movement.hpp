#pragma once

#include "odyssee/net/GameMessages.hpp"
#include "odyssee/sim/Terrain.hpp"

#include <array>
#include <cstdint>

namespace odyssee::sim {

/// Réglages du mouvement (docs/GAMEPLAY.md, chiffres provisoires, à régler en jouant).
///
/// Unités : distances en 1/256 m, vitesses en 1/256 m **par tick** (20 ticks/s), accélérations
/// en 1/256 m par tick². Exemple : 3,5 m/s = 3,5 × 256 / 20 ≈ 45.
struct MovementTuning {
    std::int32_t walkSpeed = 45;    ///< 3,5 m/s
    std::int32_t sprintSpeed = 77;  ///< 6 m/s
    std::int32_t crouchSpeed = 22;  ///< 1,7 m/s
    std::int32_t groundAccel = 26;  ///< 40 m/s² : pleine vitesse en 2 ticks
    std::int32_t airAccel = 6;      ///< contrôle réduit en l'air
    std::int32_t gravity = 6;       ///< 9,4 m/s²
    std::int32_t jumpSpeed = 61;    ///< saut d'environ 1,1 m (la gravité s'applique dès le 1er tick)
    std::int32_t maxFallSpeed = 640;///< 64 m/s
    std::int32_t stepHeight = 64;   ///< 25 cm : marche franchie sans sauter
    std::int32_t snapDown = 77;     ///< 30 cm : en descente, on reste collé au sol

    std::int32_t maxStamina = 1000;
    std::int32_t sprintCost = 6;    ///< par tick de course : ~8 s de course
    std::int32_t jumpCost = 80;
    std::int32_t regenStill = 8;    ///< par tick, à l'arrêt : ~6 s pour tout récupérer
    std::int32_t regenMoving = 4;   ///< par tick, en marchant
    std::int32_t exhaustedUntil = 200; ///< vidée, l'endurance doit remonter jusque-là pour recourir
};

inline constexpr MovementTuning kDefaultTuning{};

/// Borne des positions (± 2^30 unités, soit ± 4 194 km) : au-delà, le joueur est retenu au bord.
/// Sans elle, position + vitesse finirait par déborder d'un int32 (comportement indéfini).
inline constexpr std::int32_t kMaxCoordinate = 1 << 30;

/// État de mouvement d'un joueur (drapeaux : PlayerFlag).
enum PlayerFlag : std::uint8_t {
    kGrounded = 1U << 0,
    kCrouched = 1U << 1,
    kSprinting = 1U << 2,
    kExhausted = 1U << 3, ///< endurance vidée en courant : course impossible pour un moment
};

/// Tout ce qu'il faut pour faire avancer un joueur d'un tick. Le serveur envoie ces valeurs
/// au joueur concerné : sa prédiction repart exactement du même état (réconciliation).
struct PlayerState {
    std::array<std::int32_t, 3> position{}; ///< 1/256 m (x est, y sud, z haut) : les pieds
    std::array<std::int16_t, 3> velocity{}; ///< 1/256 m par tick
    std::uint16_t yaw = 0;                  ///< cap (net::quantizeYaw)
    std::int16_t pitch = 0;                 ///< inclinaison de la caméra (net::quantizePitch)
    std::uint16_t stamina = 1000;           ///< endurance, 0 à MovementTuning::maxStamina
    std::uint8_t flags = kGrounded;

    [[nodiscard]] bool has(PlayerFlag flag) const { return (flags & flag) != 0; }
    friend bool operator==(const PlayerState&, const PlayerState&) = default;
};

/// Avance un joueur d'un tick. Fonction pure et déterministe (entiers seulement) : même état,
/// même input et même terrain donnent le même résultat partout.
[[nodiscard]] PlayerState step(const PlayerState& state, const net::InputCommand& input,
                               const Terrain& terrain,
                               const MovementTuning& tuning = kDefaultTuning);

} // namespace odyssee::sim
