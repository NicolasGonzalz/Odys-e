#pragma once

#include "ecs/Entity.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace odyssee::server::ecs {

/// Les composants d'un même type, serrés dans un tableau (« sparse set ») : parcourir tous les
/// composants d'un type lit une mémoire contiguë, et trouver celui d'une entité se fait en un
/// accès (table indexée par l'index de l'entité).
///
/// Retirer un composant met le dernier à sa place : l'ordre n'est pas garanti.
template <typename T> class ComponentPool {
  public:
    /// Donne (ou remplace) le composant de l'entité, construit avec `args`.
    template <typename... Args> T& emplace(Entity entity, Args&&... args) {
        if (T* existing = find(entity)) {
            *existing = T(std::forward<Args>(args)...);
            return *existing;
        }
        if (entity.index >= _slotOf.size()) {
            _slotOf.resize(entity.index + std::size_t{1}, kAbsent);
        }
        _slotOf[entity.index] = static_cast<std::uint32_t>(_entities.size());
        _entities.push_back(entity);
        _components.emplace_back(std::forward<Args>(args)...);
        return _components.back();
    }

    /// Retire le composant de l'entité. Faux si elle n'en avait pas.
    bool remove(Entity entity) {
        if (!contains(entity)) {
            return false;
        }
        const std::uint32_t slot = _slotOf[entity.index];
        const std::size_t last = _entities.size() - 1;
        if (slot != last) {
            _entities[slot] = _entities[last];
            _components[slot] = std::move(_components[last]);
            _slotOf[_entities[slot].index] = slot;
        }
        _entities.pop_back();
        _components.pop_back();
        _slotOf[entity.index] = kAbsent;
        return true;
    }

    [[nodiscard]] bool contains(Entity entity) const {
        return entity.index < _slotOf.size() && _slotOf[entity.index] != kAbsent &&
               _entities[_slotOf[entity.index]] == entity; // même génération
    }

    [[nodiscard]] T* find(Entity entity) {
        return contains(entity) ? &_components[_slotOf[entity.index]] : nullptr;
    }
    [[nodiscard]] const T* find(Entity entity) const {
        return contains(entity) ? &_components[_slotOf[entity.index]] : nullptr;
    }

    [[nodiscard]] std::size_t size() const { return _entities.size(); }

    /// Les entités qui ont ce composant, et leurs composants, dans le même ordre.
    [[nodiscard]] std::span<const Entity> entities() const { return _entities; }
    [[nodiscard]] std::span<T> components() { return _components; }

  private:
    static constexpr std::uint32_t kAbsent = std::numeric_limits<std::uint32_t>::max();

    std::vector<std::uint32_t> _slotOf; ///< par index d'entité : place dans les tableaux serrés
    std::vector<Entity> _entities;
    std::vector<T> _components;
};

} // namespace odyssee::server::ecs
