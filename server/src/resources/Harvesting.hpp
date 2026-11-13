#pragma once

#include "resources/ResourceComponents.hpp"

#include "odyssee/data/ItemDef.hpp"
#include "odyssee/data/ResourceDef.hpp"

#include <cstdint>
#include <optional>

namespace odyssee::server {

/// Règles de récolte (T044), sans ECS : un coup sur une ressource, et sa repousse.
/// Les chiffres viennent de data/resources.txt (data::ResourceDef).

/// Ce qu'un coup rapporte.
struct Harvest {
    data::ItemId item;
    std::uint16_t count = 0;

    friend bool operator==(const Harvest&, const Harvest&) = default;
};

/// Outil tenu en main pour récolter ; std::nullopt : à mains nues. Tant qu'il n'y a pas
/// d'inventaire (T045), les joueurs récoltent à la main.
using HeldTool = std::optional<data::ItemId>;
inline constexpr HeldTool kBareHands = std::nullopt;

/// Une ressource pleine de ce type.
[[nodiscard]] ResourceNode freshNode(const data::ResourceDef& type);

/// Ticks entre deux coups avec cet outil : `hitTicks` avec un des outils du type (ou s'il n'en
/// demande aucun), data::kBareHandSlowdown fois plus sinon. std::nullopt si ce type ne se récolte
/// pas sans ses outils.
[[nodiscard]] std::optional<std::uint32_t> hitInterval(const data::ResourceDef& type,
                                                       HeldTool tool);

/// Un coup sur la ressource : ce qu'il rapporte, ou std::nullopt si elle est épuisée ou ne se
/// récolte pas avec cet outil. Le dernier coup l'épuise et lance sa repousse. La cadence (un coup
/// toutes les hitInterval) est à la charge de l'appelant.
[[nodiscard]] std::optional<Harvest> tryHarvest(ResourceNode& node, const data::ResourceDef& type,
                                                HeldTool tool);

/// Un tick de repousse : une ressource épuisée redevient pleine au bout de `regrowTicks`.
void stepRegrowth(ResourceNode& node, const data::ResourceDef& type);

} // namespace odyssee::server
