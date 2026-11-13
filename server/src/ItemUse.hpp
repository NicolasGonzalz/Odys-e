#pragma once

#include "Inventory.hpp"
#include "Vitals.hpp"

#include "odyssee/data/Catalog.hpp"

#include <cstddef>

namespace odyssee::server {

/// L'objet de la case `slot` (barre d'outils), s'il y en a un et que c'est un outil : ce que le
/// joueur tient pour récolter. Sinon, mains nues.
[[nodiscard]] std::optional<data::ItemId> heldToolOf(const Inventory& inventory, std::size_t slot,
                                                     const data::Catalog& catalog);

/// Utilise l'objet de la case `slot` (touche E) : un consommable est mangé ou bu (faim et soif
/// remontent de `eat` et `drink` points de %), et un exemplaire disparaît. Faux si rien à faire.
bool useItem(Inventory& inventory, std::size_t slot, Vitals& vitals, const data::Catalog& catalog);

} // namespace odyssee::server
