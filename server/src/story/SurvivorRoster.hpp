#pragma once

#include "odyssee/data/StoryDef.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace odyssee::server {

/// Qui incarne quel survivant dans ce monde (T049) : un joueur par survivant, retrouvé par son
/// nom, pour toujours (sauvegardé avec le monde).
class SurvivorRoster {
  public:
    /// Donne `survivor` au joueur `name` s'il est libre et que ce joueur n'en a pas déjà un.
    bool assign(data::SurvivorId survivor, std::string_view name);

    /// Le survivant du joueur `name`, s'il en a un.
    [[nodiscard]] std::optional<data::SurvivorId> survivorOf(std::string_view name) const;
    /// Le joueur qui incarne `survivor` (vide : personne).
    [[nodiscard]] std::string_view ownerOf(data::SurvivorId survivor) const;
    [[nodiscard]] bool taken(data::SurvivorId survivor) const { return !ownerOf(survivor).empty(); }
    /// Survivants pris, en bits (bit n = survivant n).
    [[nodiscard]] std::uint8_t takenMask() const;

  private:
    std::array<std::string, data::kMaxSurvivors> _owners;
};

} // namespace odyssee::server
