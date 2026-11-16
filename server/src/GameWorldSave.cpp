// Sauvegarde et reprise du monde (T053) : ce que GameWorld garde d'une partie à l'autre.
#include "GameWorld.hpp"

#include "odyssee/core/Log.hpp"

#include <algorithm>
#include <optional>
#include <tuple>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"sauvegarde"};

/// Les composants sauvegardés d'un joueur, tous présents, ou rien (pointeurs constants si
/// `registry` l'est).
template <typename Registry> auto playerParts(Registry& registry, ecs::Entity entity) {
    auto parts = std::tuple{
        registry.template get<PlayerControl>(entity), registry.template get<Motion>(entity),
        registry.template get<Vitals>(entity), registry.template get<Inventory>(entity),
        registry.template get<RestPoint>(entity)};
    const bool complete =
        std::apply([](const auto*... part) { return ((part != nullptr) && ...); }, parts);
    return complete ? std::optional(parts) : std::nullopt;
}

} // namespace

WorldSave GameWorld::save() const {
    WorldSave save;
    save.worldFingerprint = _worldFingerprint;
    for (const auto& [name, player] : _absent) {
        save.players.push_back(player);
    }
    for (const auto& entity : _players) {
        if (const auto player = entity ? capturePlayer(*entity) : std::nullopt) {
            save.players.push_back(*player);
        }
    }
    std::sort(save.players.begin(), save.players.end(),
              [](const SavedPlayer& a, const SavedPlayer& b) { return a.name < b.name; });

    for (const ecs::Entity node : _resources.nodes()) {
        const ResourceNode* state = _registry.get<ResourceNode>(node);
        save.resources.push_back(
            state ? SavedResource{state->type.value, state->hitsLeft, state->regrowTicksLeft}
                  : SavedResource{});
    }
    saveStory(save);
    for (const ecs::Entity bag : _bags.bags()) {
        const Placement* placement = _registry.get<Placement>(bag);
        const BagContents* contents = _registry.get<BagContents>(bag);
        if (placement && contents) {
            save.bags.push_back({placement->position, *contents});
        }
    }
    return save;
}

void GameWorld::load(const WorldSave& save) {
    if (save.players.empty() && save.resources.empty() && save.bags.empty() &&
        save.journal.empty() && save.survivors.empty()) {
        kLog.info("aucune sauvegarde : nouveau monde");
        return;
    }
    loadStory(save); // l'histoire ne dépend pas de la carte
    const bool sameMap = save.worldFingerprint == _worldFingerprint;
    for (SavedPlayer player : save.players) {
        if (!sameMap) { // une position de l'ancienne carte peut tomber dans un mur
            player.position = _spawn.position;
            player.yaw = _spawn.yaw;
            player.rest = RestPoint{_spawn.position, _spawn.yaw};
        }
        _absent[player.name] = std::move(player);
    }
    if (!sameMap) {
        kLog.warn("la carte a changé depuis la sauvegarde : joueurs remis au point d'apparition, "
                  "ressources et sacs repartent de zéro");
        return;
    }
    loadResources(save.resources);
    for (const SavedBag& bag : save.bags) {
        (void)_bags.drop(_registry, bag.position, bag.contents);
    }
    kLog.info("sauvegarde reprise : {} joueur(s), {} ressource(s), {} sac(s)", save.players.size(),
              save.resources.size(), save.bags.size());
}

void GameWorld::loadResources(const std::vector<SavedResource>& saved) {
    const std::span<const ecs::Entity> nodes = _resources.nodes();
    if (!sameResourceTypes(saved)) {
        // data/resources.txt a changé : les rangs ne désignent plus les mêmes ressources.
        kLog.warn("ressources sauvegardées différentes de celles placées ({} pour {}, ou types "
                  "changés) : ressources repartent pleines",
                  saved.size(), nodes.size());
        return;
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (ResourceNode* node = _registry.get<ResourceNode>(nodes[i])) {
            node->hitsLeft = saved[i].hitsLeft;
            node->regrowTicksLeft = saved[i].regrowTicksLeft;
        }
    }
}

bool GameWorld::sameResourceTypes(const std::vector<SavedResource>& saved) const {
    const std::span<const ecs::Entity> nodes = _resources.nodes();
    if (saved.size() != nodes.size()) {
        return false;
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const ResourceNode* node = _registry.get<ResourceNode>(nodes[i]);
        if (!node || node->type.value != saved[i].type) {
            return false;
        }
    }
    return true;
}

bool GameWorld::nameInUse(const std::string& name) const {
    return std::any_of(_players.begin(), _players.end(), [&](const auto& entity) {
        const PlayerControl* control = entity ? _registry.get<PlayerControl>(*entity) : nullptr;
        return control && control->name == name;
    });
}

std::optional<SavedPlayer> GameWorld::capturePlayer(ecs::Entity entity) const {
    const auto parts = playerParts(_registry, entity);
    if (!parts) {
        return std::nullopt;
    }
    const auto [control, motion, vitals, inventory, rest] = *parts;
    if (!control->saved) {
        return std::nullopt;
    }
    return SavedPlayer{control->name, motion->state.position, motion->state.yaw,
                       *vitals,       inventory->slots,       *rest};
}

void GameWorld::restorePlayer(ecs::Entity entity, const SavedPlayer& saved) {
    const auto parts = playerParts(_registry, entity);
    if (!parts) {
        return;
    }
    const auto [control, motion, vitals, inventory, rest] = *parts;
    motion->state = sim::PlayerState{};
    motion->state.position = saved.position;
    motion->state.yaw = saved.yaw;
    *vitals = saved.vitals;
    inventory->slots = saved.inventory;
    inventory->changed = true;
    *rest = saved.rest;
}

} // namespace odyssee::server
