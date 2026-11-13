#pragma once

#include "odyssee/data/ResourceCatalog.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <span>

namespace odyssee::debugclient {

/// Dessine les ressources récoltables (T044) reçues dans le dernier snapshot, en formes simples
/// tant qu'il n'y a pas de modèles : cylindre marron (arbre), bloc gris (rocher), touffe verte
/// (buisson), bloc sombre veiné d'orange (filon). Une ressource épuisée est plus pâle et plus
/// petite. Elles ne bougent pas : on les affiche à la dernière position connue, sans interpoler.
///
/// `types` : data/resources.txt, le même que celui du serveur (le type circule par son rang) ;
/// nul ou type inconnu : un simple bloc violet.
void drawResources(std::span<const net::EntityState> entities, const data::ResourceCatalog* types);

} // namespace odyssee::debugclient
