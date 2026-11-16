#include "story/SurvivorRoster.hpp"

namespace odyssee::server {

bool SurvivorRoster::assign(data::SurvivorId survivor, std::string_view name) {
    if (survivor.value >= _owners.size() || name.empty() || taken(survivor) || survivorOf(name)) {
        return false;
    }
    _owners[survivor.value] = std::string(name);
    return true;
}

std::optional<data::SurvivorId> SurvivorRoster::survivorOf(std::string_view name) const {
    for (std::size_t i = 0; i < _owners.size(); ++i) {
        if (!name.empty() && _owners[i] == name) {
            return data::SurvivorId{static_cast<std::uint8_t>(i)};
        }
    }
    return std::nullopt;
}

std::string_view SurvivorRoster::ownerOf(data::SurvivorId survivor) const {
    return survivor.value < _owners.size() ? std::string_view(_owners[survivor.value])
                                           : std::string_view();
}

std::uint8_t SurvivorRoster::takenMask() const {
    std::uint8_t mask = 0;
    for (std::size_t i = 0; i < _owners.size(); ++i) {
        if (!_owners[i].empty()) {
            mask = static_cast<std::uint8_t>(mask | (1U << i));
        }
    }
    return mask;
}

} // namespace odyssee::server
