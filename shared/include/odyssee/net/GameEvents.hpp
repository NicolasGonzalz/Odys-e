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
/// et dans l'ordre, mais pas à chaque tick. Pour l'instant : l'inventaire (T045).

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

enum class GameEventType : std::uint8_t {
    InventoryContents = 1,
    MoveItem = 2,
    CraftRequest = 3,
    CraftStatus = 4,
};

using GameEvent = std::variant<InventoryContents, MoveItem, CraftRequest, CraftStatus>;

[[nodiscard]] std::vector<std::byte> encodeGameEvent(const GameEvent& message);

/// Relit un message fiable. std::nullopt s'il est invalide (type inconnu, case hors limites,
/// quantité nulle avec un objet, octets en trop…).
[[nodiscard]] std::optional<GameEvent> decodeGameEvent(std::span<const std::byte> bytes);

} // namespace odyssee::net
