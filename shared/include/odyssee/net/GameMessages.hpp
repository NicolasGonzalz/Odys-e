#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace odyssee::net {

/// Numéro de tick de simulation. À 20 ticks/s, 32 bits durent 6 ans : pas de rebouclage.
using Tick = std::uint32_t;

/// Numéro d'une entité du monde (joueur, créature, objet…) dans les messages réseau.
using EntityId = std::uint16_t;

// --- Client → serveur : inputs -----------------------------------------------------------------

/// Touches d'action, en champ de bits. Valeurs figées : on ajoute, on ne renumérote jamais.
enum class InputButton : std::uint16_t {
    Jump = 1U << 0,
    Sprint = 1U << 1,
    Crouch = 1U << 2,
    Use = 1U << 3,       ///< interagir, récolter
    Primary = 1U << 4,   ///< action principale (frapper, utiliser l'outil)
    Secondary = 1U << 5, ///< action secondaire (viser, parer)
};

/// Tous les bits connus : un input avec un autre bit est refusé.
inline constexpr std::uint16_t kKnownInputButtons = 0x003F;

/// Ce que le joueur demande pendant un tick, déjà quantifié (voir Quantize.hpp) : le client
/// simule avec exactement ces valeurs, comme le serveur.
struct InputCommand {
    std::int8_t moveForward = 0; ///< -127 (recule) à 127 (avance)
    std::int8_t moveRight = 0;   ///< -127 (gauche) à 127 (droite)
    std::uint16_t yaw = 0;       ///< cap de la caméra (quantizeYaw)
    std::int16_t pitch = 0;      ///< inclinaison de la caméra (quantizePitch)
    std::uint16_t buttons = 0;   ///< InputButton combinés

    [[nodiscard]] bool isPressed(InputButton button) const {
        return (buttons & static_cast<std::uint16_t>(button)) != 0;
    }
    friend bool operator==(const InputCommand&, const InputCommand&) = default;
};

/// Inputs de ticks consécutifs, du plus ancien au plus récent : celui du tick courant et ceux qui
/// précèdent (au plus kMaxInputsPerBatch). Message non fiable : la redondance remplace le renvoi.
/// Les ticks d'input commencent à 1 : 0 veut dire « aucun » (Snapshot::lastProcessedInput).
struct InputBatch {
    Tick firstTick = 0;
    std::vector<InputCommand> commands;

    [[nodiscard]] Tick tickAt(std::size_t index) const {
        return firstTick + static_cast<Tick>(index);
    }
    friend bool operator==(const InputBatch&, const InputBatch&) = default;
};

// --- Serveur → client : état du monde ----------------------------------------------------------

/// Sorte d'entité. Valeurs figées : on ajoute, on ne renumérote jamais.
enum class EntityKind : std::uint8_t {
    Player = 1,
};

/// État d'une entité tel que vu par les clients (positions et cap quantifiés).
struct EntityState {
    EntityId id = 0;
    EntityKind kind = EntityKind::Player;
    std::array<std::int32_t, 3> position{}; ///< quantizePosition : x est, y sud, z haut
    std::uint16_t yaw = 0;                  ///< quantizeYaw
    std::uint8_t flags = 0;                 ///< animation (au sol, accroupi…) : défini en M2

    friend bool operator==(const EntityState&, const EntityState&) = default;
};

/// Ce que la simulation du joueur destinataire contient en plus de son entité (position, cap,
/// drapeaux) : de quoi reprendre sa prédiction exactement là où le serveur l'a laissée.
struct OwnMotion {
    std::array<std::int16_t, 3> velocity{}; ///< 1/256 m par tick (voir sim::PlayerState)
    std::uint16_t stamina = 0;

    friend bool operator==(const OwnMotion&, const OwnMotion&) = default;
};

/// Les besoins vitaux du joueur destinataire (T051), en pourcentage (0 à kMaxVital). Calculés par
/// le serveur seul : le client ne fait que les afficher.
inline constexpr std::uint8_t kMaxVital = 100;

struct OwnVitals {
    std::uint8_t health = kMaxVital;
    std::uint8_t hunger = kMaxVital; ///< 100 = rassasié, 0 = affamé
    std::uint8_t thirst = kMaxVital; ///< 100 = désaltéré, 0 = assoiffé

    friend bool operator==(const OwnVitals&, const OwnVitals&) = default;
};

/// L'état du monde vu par un joueur à un tick du serveur.
struct Snapshot {
    Tick serverTick = 0;
    /// Dernier input de **ce** joueur appliqué par le serveur : le client rejoue les suivants
    /// (réconciliation). 0 tant qu'aucun input n'a été reçu.
    Tick lastProcessedInput = 0;
    std::vector<EntityState> entities; ///< au plus kMaxSnapshotEntities
    OwnMotion self;                    ///< le joueur destinataire
    OwnVitals vitals;                  ///< ses besoins vitaux

    friend bool operator==(const Snapshot&, const Snapshot&) = default;
};

// --- Encodage ---------------------------------------------------------------------------------

/// Type d'un message de jeu, écrit devant son contenu dans un paquet de jeu. Valeurs figées.
enum class GameMessageType : std::uint8_t {
    Inputs = 1,   ///< InputBatch, client → serveur
    Snapshot = 2, ///< Snapshot, serveur → client
};

using GameMessage = std::variant<InputBatch, Snapshot>;

[[nodiscard]] GameMessageType typeOf(const GameMessage& message);

/// Contenu d'un message (sans son type).
[[nodiscard]] std::vector<std::byte> encodeGameMessage(const GameMessage& message);

/// Relit un message à partir de son type (octet brut, éventuellement inconnu) et de son contenu.
/// std::nullopt si le type est inconnu, le contenu invalide ou suivi d'octets en trop.
[[nodiscard]] std::optional<GameMessage> decodeGameMessage(std::uint8_t type,
                                                           std::span<const std::byte> body);

} // namespace odyssee::net
