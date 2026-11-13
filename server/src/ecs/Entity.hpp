#pragma once

#include <cstdint>

namespace odyssee::server::ecs {

/// Une entité du monde (joueur, ressource, créature, objet au sol…) : rien qu'un identifiant.
/// Ses données sont dans des composants (ComponentPool), rangés à part, type par type.
///
/// `index` est réutilisé quand une entité disparaît ; `generation` change alors. Un identifiant
/// gardé trop longtemps (entité détruite puis place reprise) ne désigne donc jamais la nouvelle.
struct Entity {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const Entity&, const Entity&) = default;
};

} // namespace odyssee::server::ecs
