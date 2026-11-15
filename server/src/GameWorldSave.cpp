// Sauvegarde et reprise du monde (T053) : ce que GameWorld garde d'une partie à l'autre.
#include "GameWorld.hpp"

#include "odyssee/core/Log.hpp"

#include <algorithm>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"sauvegarde"};

} // namespace

WorldSave GameWorld::save() const {
    WorldSave save;
    save.worldFingerprint = _worldFingerprint;
    for (const auto& [name, player] : _absent) {
        save.players.push_back(player);
    }
    for (const auto& entity : _players) {
        if (entity && _registry.get<PlayerControl>(*entity)->saved) {
            save.players.push_back(capturePlayer(*entity));
        }
    }
    std::sort(save.players.begin(), save.players.end(),
              [](const SavedPlayer& a, const SavedPlayer& b) { return a.name < b.name; });

    for (const ecs::Entity node : _resources.nodes()) {
        const ResourceNode& state = *_registry.get<ResourceNode>(node);
        save.resources.push_back({state.hitsLeft, state.regrowTicksLeft});
    }
    for (const ecs::Entity bag : _bags.bags()) {
        save.bags.push_back(
            {_registry.get<Placement>(bag)->position, *_registry.get<BagContents>(bag)});
    }
    return save;
}

void GameWorld::load(const WorldSave& save) {
    if (save.players.empty() && save.resources.empty() && save.bags.empty()) {
        kLog.info("aucune sauvegarde : nouveau monde");
        return;
    }
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
    if (saved.size() != nodes.size()) {
        // data/resources.txt a changé : les rangs ne désignent plus les mêmes ressources.
        kLog.warn("{} ressource(s) sauvegardée(s) pour {} placée(s) : ressources repartent pleines",
                  saved.size(), nodes.size());
        return;
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        ResourceNode& node = *_registry.get<ResourceNode>(nodes[i]);
        node.hitsLeft = saved[i].hitsLeft;
        node.regrowTicksLeft = saved[i].regrowTicksLeft;
    }
}

bool GameWorld::nameInUse(const std::string& name) const {
    return std::any_of(_players.begin(), _players.end(), [&](const auto& entity) {
        return entity && _registry.get<PlayerControl>(*entity)->name == name;
    });
}

SavedPlayer GameWorld::capturePlayer(ecs::Entity entity) const {
    const sim::PlayerState& state = _registry.get<Motion>(entity)->state;
    SavedPlayer saved;
    saved.name = _registry.get<PlayerControl>(entity)->name;
    saved.position = state.position;
    saved.yaw = state.yaw;
    saved.vitals = *_registry.get<Vitals>(entity);
    saved.inventory = _registry.get<Inventory>(entity)->slots;
    saved.rest = *_registry.get<RestPoint>(entity);
    return saved;
}

void GameWorld::restorePlayer(ecs::Entity entity, const SavedPlayer& saved) {
    sim::PlayerState& state = _registry.get<Motion>(entity)->state;
    state = sim::PlayerState{};
    state.position = saved.position;
    state.yaw = saved.yaw;
    *_registry.get<Vitals>(entity) = saved.vitals;
    Inventory& inventory = *_registry.get<Inventory>(entity);
    inventory.slots = saved.inventory;
    inventory.changed = true;
    *_registry.get<RestPoint>(entity) = saved.rest;
}

} // namespace odyssee::server
