#pragma once

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

/// Cases d'un inventaire : les 5 premières sont la barre d'outils (touches 1 à 5).
inline constexpr std::size_t kInventorySlots = 25;
inline constexpr std::size_t kHotbarSlots = 5;

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

enum class GameEventType : std::uint8_t {
    InventoryContents = 1,
    MoveItem = 2,
};

using GameEvent = std::variant<InventoryContents, MoveItem>;

[[nodiscard]] std::vector<std::byte> encodeGameEvent(const GameEvent& message);

/// Relit un message fiable. std::nullopt s'il est invalide (type inconnu, case hors limites,
/// quantité nulle avec un objet, octets en trop…).
[[nodiscard]] std::optional<GameEvent> decodeGameEvent(std::span<const std::byte> bytes);

} // namespace odyssee::net
