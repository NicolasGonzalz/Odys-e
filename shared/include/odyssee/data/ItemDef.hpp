#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace odyssee::data {

/// Objet du jeu, désigné par un indice compact attribué au chargement (ordre du fichier) : c'est
/// lui qui circulera dans la simulation et sur le réseau. L'identifiant texte (`wood_log`) reste
/// la référence stable entre fichiers, sauvegardes et client Unreal.
struct ItemId {
    std::uint16_t value = 0;

    friend auto operator<=>(const ItemId&, const ItemId&) = default;
};

enum class ItemCategory : std::uint8_t {
    Resource,   ///< matière première ou intermédiaire (bûche, planches, lingot…)
    Tool,       ///< outil tenu en main (hache, pioche, torche…)
    Weapon,     ///< arme
    Ammo,       ///< munitions
    Station,    ///< poste posé dans le monde (établi, forge, coffre…)
    Consumable, ///< se consomme (nourriture, soins, piles…)
    Equipment,  ///< objet porté ou consulté (sac à dos, carte, journal…)
};

/// « resource », « tool »… → catégorie (nullopt si inconnue).
[[nodiscard]] std::optional<ItemCategory> parseItemCategory(std::string_view text);
[[nodiscard]] std::string_view itemCategoryName(ItemCategory category);

/// Description d'un objet, telle que lue dans data/items.txt.
struct ItemDef {
    ItemId id;
    std::string key;            ///< identifiant texte stable : `stone_axe`
    std::string name;           ///< nom affiché (français, UTF-8)
    std::uint16_t maxStack = 1; ///< taille de pile maximale (≥ 1)
    ItemCategory category = ItemCategory::Resource;
    std::string model; ///< modèle 3D, relatif à art/exports, sans extension (vide si aucun)
    /// Consommable : ce qu'il rend de faim et de soif, en points de % (0 à 100 ; T047).
    std::uint8_t eat = 0;
    std::uint8_t drink = 0;
};

} // namespace odyssee::data
