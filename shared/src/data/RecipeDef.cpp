#include "odyssee/data/RecipeDef.hpp"

#include <array>
#include <cstddef>

namespace odyssee::data {

namespace {

/// Noms des postes dans les fichiers, dans l'ordre de l'énumération.
constexpr std::array<std::string_view, 3> kStationNames{"hand", "workbench", "forge"};

} // namespace

std::optional<Station> parseStation(std::string_view text) {
    for (std::size_t i = 0; i < kStationNames.size(); ++i) {
        if (kStationNames[i] == text) {
            return static_cast<Station>(i);
        }
    }
    return std::nullopt;
}

std::string_view stationName(Station station) {
    const auto index = static_cast<std::size_t>(station);
    return index < kStationNames.size() ? kStationNames[index] : std::string_view{};
}

} // namespace odyssee::data
