#pragma once

#include "ecs/Entity.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace odyssee::server::ecs {

/// Distribue les identifiants d'entité et sait lesquels sont vivants.
class EntityPool {
  public:
    /// Une nouvelle entité (une place libérée est réutilisée, avec une nouvelle génération).
    [[nodiscard]] Entity create();

    /// Libère l'entité. Sans effet si elle n'est pas (ou plus) vivante.
    void destroy(Entity entity);

    [[nodiscard]] bool alive(Entity entity) const;
    [[nodiscard]] std::size_t size() const { return _generations.size() - _free.size(); }

  private:
    std::vector<std::uint32_t> _generations; ///< génération courante de chaque place
    std::vector<bool> _used;                 ///< place occupée par une entité vivante
    std::vector<std::uint32_t> _free;        ///< places libres, réutilisées d'abord
};

} // namespace odyssee::server::ecs
