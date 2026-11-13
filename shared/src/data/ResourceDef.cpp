#include "odyssee/data/ResourceDef.hpp"

#include <array>
#include <cstddef>

namespace odyssee::data {

namespace {

/// Noms des formes dans les fichiers, dans l'ordre de l'énumération.
constexpr std::array<std::string_view, 4> kShapeNames{"tree", "rock", "bush", "vein"};

} // namespace

std::optional<ResourceShape> parseResourceShape(std::string_view text) {
    for (std::size_t i = 0; i < kShapeNames.size(); ++i) {
        if (kShapeNames[i] == text) {
            return static_cast<ResourceShape>(i);
        }
    }
    return std::nullopt;
}

std::string_view resourceShapeName(ResourceShape shape) {
    const auto index = static_cast<std::size_t>(shape);
    return index < kShapeNames.size() ? kShapeNames[index] : std::string_view{};
}

} // namespace odyssee::data
