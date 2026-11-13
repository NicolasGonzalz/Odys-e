#pragma once

#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace odyssee::server {

/// Composant : l'inventaire d'un joueur (T045), géré par le serveur seul. Le client en reçoit une
/// copie à chaque changement (net::InventoryContents) et ne fait que demander des déplacements.
struct Inventory {
    std::array<net::InventorySlot, net::kInventorySlots> slots{};
    bool changed = true; ///< à renvoyer au joueur (vrai à l'apparition : il reçoit son inventaire)
};

/// Ajoute `count` objets : d'abord sur les piles du même objet, puis dans les cases vides (barre
/// d'outils en premier). Renvoie ce qui n'a pas trouvé de place. Objet inconnu : rien n'est pris.
std::uint16_t addItem(Inventory& inventory, data::ItemId item, std::uint16_t count,
                      const data::Catalog& catalog);

/// Retire `count` objets s'il y en a assez (sinon rien ne change et renvoie faux).
bool removeItem(Inventory& inventory, data::ItemId item, std::uint32_t count);

/// Nombre total de cet objet dans l'inventaire.
[[nodiscard]] std::uint32_t countItem(const Inventory& inventory, data::ItemId item);

/// Déplace une case vers une autre : regroupe si c'est le même objet (dans la limite de la pile),
/// sinon échange. Faux (rien ne change) si une case est hors limites ou si `from` est vide.
bool moveItem(Inventory& inventory, std::size_t from, std::size_t to, const data::Catalog& catalog);

/// La copie envoyée au joueur.
[[nodiscard]] net::InventoryContents toContents(const Inventory& inventory);

} // namespace odyssee::server
