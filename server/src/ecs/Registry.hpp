#pragma once

#include "ecs/ComponentPool.hpp"
#include "ecs/EntityPool.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace odyssee::server::ecs {

namespace detail {

/// Numéro unique de chaque type de composant, attribué à sa première utilisation (sans RTTI).
inline std::size_t nextComponentTypeId() {
    static std::size_t next = 0;
    return next++;
}

template <typename T> std::size_t componentTypeId() {
    static const std::size_t id = nextComponentTypeId();
    return id;
}

} // namespace detail

/// Le monde du serveur vu comme un ECS : des entités, et leurs composants rangés par type.
/// Les « systèmes » sont de simples fonctions qui, à chaque tick, parcourent les entités ayant
/// les composants qui les intéressent (each).
///
/// Un composant est n'importe quel type copiable ou déplaçable (struct de données, sans logique).
class Registry {
  public:
    [[nodiscard]] Entity create() { return _entities.create(); }

    /// Détruit l'entité et retire tous ses composants. Sans effet si elle n'est plus vivante.
    void destroy(Entity entity) {
        if (!_entities.alive(entity)) {
            return;
        }
        for (auto& pool : _pools) {
            if (pool) {
                pool->remove(entity);
            }
        }
        _entities.destroy(entity);
    }

    [[nodiscard]] bool alive(Entity entity) const { return _entities.alive(entity); }
    [[nodiscard]] std::size_t size() const { return _entities.size(); }

    /// Donne (ou remplace) un composant à une entité vivante.
    template <typename T, typename... Args> T& add(Entity entity, Args&&... args) {
        return pool<T>().emplace(entity, std::forward<Args>(args)...);
    }

    template <typename T> bool remove(Entity entity) { return pool<T>().remove(entity); }

    /// Le composant de l'entité, ou nullptr.
    template <typename T> [[nodiscard]] T* get(Entity entity) { return pool<T>().find(entity); }
    template <typename T> [[nodiscard]] const T* get(Entity entity) const {
        const ComponentPool<T>* components = findPool<T>();
        return components ? components->find(entity) : nullptr;
    }
    template <typename T> [[nodiscard]] bool has(Entity entity) const {
        return get<T>(entity) != nullptr;
    }

    /// Appelle `fn(entity, First&, Rest&...)` pour chaque entité qui a tous ces composants.
    ///
    /// Parcours à l'envers : `fn` peut retirer les composants de l'entité en cours, ou la
    /// détruire. Il ne doit pas ajouter de composant d'un type parcouru.
    template <typename First, typename... Rest, typename Fn> void each(Fn&& fn) {
        ComponentPool<First>& firsts = pool<First>();
        for (std::size_t i = firsts.size(); i-- > 0;) {
            if (i >= firsts.size()) {
                continue; // plusieurs entités retirées par fn
            }
            const Entity entity = firsts.entities()[i];
            if ((pool<Rest>().contains(entity) && ...)) {
                fn(entity, firsts.components()[i], *pool<Rest>().find(entity)...);
            }
        }
    }

    /// Tous les composants d'un type (parcours direct, le plus rapide).
    template <typename T> [[nodiscard]] ComponentPool<T>& pool() {
        const std::size_t id = detail::componentTypeId<T>();
        if (id >= _pools.size()) {
            _pools.resize(id + 1);
        }
        if (!_pools[id]) {
            _pools[id] = std::make_unique<TypedPool<T>>();
        }
        return static_cast<TypedPool<T>&>(*_pools[id]).components;
    }

  private:
    /// Le tableau d'un type, s'il existe déjà (lecture seule : rien n'est créé).
    template <typename T> [[nodiscard]] const ComponentPool<T>* findPool() const {
        const std::size_t id = detail::componentTypeId<T>();
        if (id >= _pools.size() || !_pools[id]) {
            return nullptr;
        }
        return &static_cast<const TypedPool<T>&>(*_pools[id]).components;
    }

    /// Pour retirer les composants d'une entité détruite sans connaître leurs types.
    struct AnyPool {
        virtual ~AnyPool() = default;
        virtual void remove(Entity entity) = 0;
    };
    template <typename T> struct TypedPool final : AnyPool {
        ComponentPool<T> components;
        void remove(Entity entity) override { components.remove(entity); }
    };

    EntityPool _entities;
    std::vector<std::unique_ptr<AnyPool>> _pools;
};

} // namespace odyssee::server::ecs
