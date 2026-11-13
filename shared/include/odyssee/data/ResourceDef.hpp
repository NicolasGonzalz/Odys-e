#pragma once

#include "odyssee/data/ItemDef.hpp"
#include "odyssee/data/RecipeDef.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

/// Type de ressource récoltable, désigné par un indice compact attribué au chargement (ordre de
/// data/resources.txt). Il circule sur le réseau (net::kResourceTypeMask) : client et serveur
/// doivent donc lire le même fichier. L'identifiant texte (`tree`) reste la référence stable.
struct ResourceTypeId {
    std::uint8_t value = 0;

    friend auto operator<=>(const ResourceTypeId&, const ResourceTypeId&) = default;
};

/// Types au plus : l'indice tient sur 7 bits dans les drapeaux d'entité (net::kResourceTypeMask).
inline constexpr std::size_t kMaxResourceTypes = 128;

/// Forme provisoire affichée par les clients tant qu'il n'y a pas de modèle 3D.
enum class ResourceShape : std::uint8_t {
    Tree, ///< tronc (cylindre)
    Rock, ///< bloc
    Bush, ///< touffe basse
    Vein, ///< filon : bloc veiné, plus sombre
};

/// « tree », « rock », « bush », « vein » → forme (nullopt si inconnue).
[[nodiscard]] std::optional<ResourceShape> parseResourceShape(std::string_view text);
[[nodiscard]] std::string_view resourceShapeName(ResourceShape shape);

/// À la main (sans aucun des outils indiqués), un coup prend ce nombre de fois plus longtemps.
inline constexpr std::uint32_t kBareHandSlowdown = 2;

/// Un type de ressource récoltable (arbre, rocher…), tel que lu dans data/resources.txt.
///
/// Règle de récolte : chaque coup rapporte `yield` ; après `hits` coups, la ressource est
/// épuisée et repousse (pleine) `regrowTicks` plus tard. Avec un des `tools`, un coup toutes les
/// `hitTicks` ; à la main, kBareHandSlowdown fois plus lent, ou impossible si `handAllowed` est
/// faux.
struct ResourceDef {
    ResourceTypeId id;
    std::string key;  ///< identifiant texte stable : `tree`
    std::string name; ///< nom affiché (français, UTF-8)
    ResourceShape shape = ResourceShape::Tree;
    ItemStack yield;               ///< objet et quantité gagnés à chaque coup
    std::uint16_t hits = 1;        ///< coups avant épuisement
    std::uint32_t hitTicks = 1;    ///< ticks entre deux coups, avec l'outil (20 ticks = 1 s)
    std::vector<ItemId> tools;     ///< outils qui récoltent à pleine vitesse (vide : la main)
    bool handAllowed = true;       ///< récoltable sans outil
    std::uint32_t regrowTicks = 1; ///< durée de repousse une fois épuisée
    std::uint16_t abundance = 0;   ///< poids relatif au placement (0 : jamais placée d'office)
};

} // namespace odyssee::data
