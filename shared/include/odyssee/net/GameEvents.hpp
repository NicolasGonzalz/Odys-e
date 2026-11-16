#pragma once

#include "odyssee/core/Config.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace odyssee::net {

/// Événements de jeu, envoyés sur le canal fiable ordonné (docs/NETWORK.md) : ce qui doit arriver,
/// et dans l'ordre, mais pas à chaque tick : l'inventaire (T045), l'artisanat (T047), la mort
/// (T052), les survivants et le journal (T049), la carte (T074).

using odyssee::kHotbarSlots;
using odyssee::kInventorySlots;

/// Une case : un objet (indice du catalogue, data::ItemId) et sa quantité. 0 = case vide.
struct InventorySlot {
    std::uint16_t item = 0;
    std::uint16_t count = 0;

    friend bool operator==(const InventorySlot&, const InventorySlot&) = default;
};

/// Serveur → client : tout l'inventaire du joueur, envoyé à chaque changement (100 octets).
struct InventoryContents {
    std::array<InventorySlot, kInventorySlots> slots{};

    friend bool operator==(const InventoryContents&, const InventoryContents&) = default;
};

/// Client → serveur : déplacer le contenu d'une case vers une autre (échange, ou regroupement
/// d'une même pile). Le serveur vérifie et renvoie l'inventaire tel qu'il est vraiment.
struct MoveItem {
    std::uint8_t from = 0;
    std::uint8_t to = 0;

    friend bool operator==(const MoveItem&, const MoveItem&) = default;
};

/// Client → serveur : fabriquer la recette de rang `recipe` (data/recipes.txt, T047).
struct CraftRequest {
    std::uint16_t recipe = 0;

    friend bool operator==(const CraftRequest&, const CraftRequest&) = default;
};

/// Ce que devient une demande de fabrication. Valeurs figées (lues par d'autres versions).
enum class CraftOutcome : std::uint8_t {
    Started = 1,            ///< ingrédients pris ; finie dans `ticks`
    Done = 2,               ///< résultat ajouté à l'inventaire
    Busy = 3,               ///< une autre fabrication est en cours
    NeedStation = 4,        ///< il faut un établi ou une forge
    MissingIngredients = 5, ///< pas assez d'ingrédients
    UnknownRecipe = 6,
};

/// Serveur → client : réponse à une demande, puis fin de la fabrication.
struct CraftStatus {
    std::uint16_t recipe = 0;
    CraftOutcome outcome = CraftOutcome::Started;
    std::uint16_t ticks = 0; ///< durée de la fabrication (Started), sinon 0

    friend bool operator==(const CraftStatus&, const CraftStatus&) = default;
};

/// Serveur → client : le joueur est mort (T052) et réapparaît au point de repos. `bag` est le
/// numéro d'entité du sac laissé sur place avec son inventaire, 0 s'il n'avait rien à laisser.
struct PlayerDied {
    std::uint16_t bag = 0;

    friend bool operator==(const PlayerDied&, const PlayerDied&) = default;
};

/// Pas de survivant (data/story.txt) : valeur réservée des champs « survivant ».
inline constexpr std::uint8_t kNoSurvivor = 0xFF;

/// Client → serveur : incarner ce survivant (rang dans data/story.txt), s'il est libre (T049).
struct ChooseSurvivor {
    std::uint8_t survivor = 0;

    friend bool operator==(const ChooseSurvivor&, const ChooseSurvivor&) = default;
};

/// Serveur → client : son survivant (kNoSurvivor tant qu'il n'en a pas choisi), ceux déjà pris
/// dans ce monde (bit n = survivant n), et s'il vient de se réveiller (écran de la capsule).
struct SurvivorStatus {
    std::uint8_t yours = kNoSurvivor;
    std::uint8_t taken = 0;
    bool awakening = false;

    friend bool operator==(const SurvivorStatus&, const SurvivorStatus&) = default;
};

/// Quelle version d'un indice le journal contient.
enum class ClueVersion : std::uint8_t {
    Full = 1,    ///< le texte complet (et, pour un indice personnel lu par son survivant, le souvenir)
    Generic = 2, ///< indice personnel dont le survivant n'est joué par personne
};

/// Serveur → client : un indice entre dans son journal (rang dans data/story.txt), et qui l'a
/// trouvé. Tout le journal est renvoyé ainsi à la connexion.
struct JournalEntry {
    std::uint16_t clue = 0;
    ClueVersion version = ClueVersion::Full;
    std::uint8_t finder = kNoSurvivor; ///< survivant qui l'a lu

    friend bool operator==(const JournalEntry&, const JournalEntry&) = default;
};

/// Serveur → client : cet indice personnel appartient à un autre survivant, joué dans ce monde.
struct ClueLocked {
    std::uint16_t clue = 0;
    std::uint8_t owner = 0; ///< survivant qui saurait le lire

    friend bool operator==(const ClueLocked&, const ClueLocked&) = default;
};

/// Un carré de carte de kMapChunkMeters de côté (T074), en coordonnées de carré : x vers l'est,
/// y vers le sud, (0, 0) au coin nord-ouest du monde.
struct MapChunk {
    std::int16_t x = 0;
    std::int16_t y = 0;

    friend auto operator<=>(const MapChunk&, const MapChunk&) = default;
};

inline constexpr std::int32_t kMapChunkMeters = 64;
/// Carrés par événement MapChunks : un lot tient largement dans un paquet.
inline constexpr std::size_t kMaxMapChunksPerEvent = 200;

/// Les deux couches de la carte d'un joueur.
enum class MapLayer : std::uint8_t {
    Shared = 1,  ///< la carte commune du monde, reportée aux tables de cartographie
    Pending = 2, ///< ses souvenirs : vus en marchant, pas encore reportés
};

/// Serveur → client : des carrés de carte à ajouter à une couche ; `reset` vide d'abord la couche
/// (à la connexion, et quand ses souvenirs viennent d'être reportés).
struct MapChunks {
    MapLayer layer = MapLayer::Shared;
    bool reset = false;
    std::vector<MapChunk> chunks; ///< au plus kMaxMapChunksPerEvent

    friend bool operator==(const MapChunks&, const MapChunks&) = default;
};

enum class GameEventType : std::uint8_t {
    InventoryContents = 1,
    MoveItem = 2,
    CraftRequest = 3,
    CraftStatus = 4,
    PlayerDied = 5,
    ChooseSurvivor = 6,
    SurvivorStatus = 7,
    JournalEntry = 8,
    ClueLocked = 9,
    MapChunks = 10,
};

using GameEvent = std::variant<InventoryContents, MoveItem, CraftRequest, CraftStatus, PlayerDied,
                               ChooseSurvivor, SurvivorStatus, JournalEntry, ClueLocked, MapChunks>;

[[nodiscard]] std::vector<std::byte> encodeGameEvent(const GameEvent& message);

/// Relit un message fiable. std::nullopt s'il est invalide (type inconnu, case hors limites,
/// quantité nulle avec un objet, octets en trop…).
[[nodiscard]] std::optional<GameEvent> decodeGameEvent(std::span<const std::byte> bytes);

} // namespace odyssee::net
