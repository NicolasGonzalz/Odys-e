#include "resources/ResourceScatter.hpp"

#include <algorithm>
#include <cstdlib>
#include <optional>

namespace odyssee::server {

namespace {

/// Générateur pseudo-aléatoire SplitMix64 : entiers seulement, même suite sur tous les
/// compilateurs (std::uniform_int_distribution, lui, varie d'une bibliothèque à l'autre).
class ScatterRandom {
  public:
    explicit ScatterRandom(std::uint64_t seed) : _state(seed) {}

    std::uint64_t next() {
        _state += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z = _state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    /// Entier de [0, bound[ (bound > 0).
    std::uint64_t below(std::uint64_t bound) { return next() % bound; }

    /// Entier de [-half, half].
    std::int32_t centered(std::int32_t half) {
        const auto span = static_cast<std::uint64_t>(half) * 2 + 1;
        return static_cast<std::int32_t>(static_cast<std::int64_t>(below(span)) - half);
    }

  private:
    std::uint64_t _state;
};

/// Le sol en (x, y) s'il convient (voir ScatterSettings), sinon std::nullopt.
std::optional<std::int32_t> openGroundAt(const sim::Terrain& terrain, std::int32_t x,
                                         std::int32_t y) {
    const std::int32_t z = terrain.groundHeight(x, y);
    if (z == sim::kNoGround || terrain.isSolid(x, y, z - 64, z + kClearance)) {
        return std::nullopt; // trou, mur, plafond, ou dessus d'un volume (toit, plancher)
    }
    constexpr std::array<std::array<std::int32_t, 2>, 4> kAround{
        {{256, 0}, {-256, 0}, {0, 256}, {0, -256}}};
    for (const auto& [dx, dy] : kAround) {
        const std::int32_t near = terrain.groundHeight(x + dx, y + dy);
        if (near == sim::kNoGround || std::abs(near - z) > kMaxRise) {
            return std::nullopt; // pente raide, ou bord d'un trou
        }
    }
    return z;
}

bool isInRing(std::int32_t dx, std::int32_t dy, const ScatterSettings& settings) {
    const std::int64_t distanceSquared = std::int64_t{dx} * dx + std::int64_t{dy} * dy;
    return distanceSquared >= std::int64_t{settings.minRadius} * settings.minRadius &&
           distanceSquared <= std::int64_t{settings.maxRadius} * settings.maxRadius;
}

bool isTooClose(const std::vector<PlacedResource>& placed, std::int32_t x, std::int32_t y,
                std::int32_t spacing) {
    const std::int64_t minSquared = std::int64_t{spacing} * spacing;
    return std::any_of(placed.begin(), placed.end(), [&](const PlacedResource& other) {
        const std::int64_t dx = std::int64_t{other.position[0]} - x;
        const std::int64_t dy = std::int64_t{other.position[1]} - y;
        return dx * dx + dy * dy < minSquared;
    });
}

/// Un type au hasard, chacun avec une chance proportionnelle à son abondance (total > 0).
data::ResourceTypeId pickType(ScatterRandom& random, std::span<const data::ResourceDef> types,
                              std::uint64_t totalAbundance) {
    std::uint64_t roll = random.below(totalAbundance);
    for (const data::ResourceDef& type : types) {
        if (roll < type.abundance) {
            return type.id;
        }
        roll -= type.abundance;
    }
    return types.back().id; // inatteignable : roll < somme des abondances
}

std::uint64_t totalAbundanceOf(std::span<const data::ResourceDef> types) {
    std::uint64_t total = 0;
    for (const data::ResourceDef& type : types) {
        total += type.abundance;
    }
    return total;
}

} // namespace

std::vector<PlacedResource> scatterResources(const sim::Terrain& terrain,
                                             const std::array<std::int32_t, 3>& center,
                                             std::span<const data::ResourceDef> types,
                                             const ScatterSettings& settings) {
    std::vector<PlacedResource> placed;
    const std::uint64_t totalAbundance = totalAbundanceOf(types);
    if (totalAbundance == 0) {
        return placed;
    }

    ScatterRandom random(settings.seed);
    const std::uint64_t attempts = std::uint64_t{settings.count} * settings.attemptsPerNode;
    for (std::uint64_t i = 0; i < attempts && placed.size() < settings.count; ++i) {
        const std::int32_t dx = random.centered(settings.maxRadius);
        const std::int32_t dy = random.centered(settings.maxRadius);
        const std::int32_t x = center[0] + dx;
        const std::int32_t y = center[1] + dy;
        if (!isInRing(dx, dy, settings) || isTooClose(placed, x, y, settings.minSpacing)) {
            continue;
        }
        if (const auto z = openGroundAt(terrain, x, y)) {
            const data::ResourceTypeId type = pickType(random, types, totalAbundance);
            const auto yaw = static_cast<std::uint16_t>(random.below(65536));
            placed.push_back(PlacedResource{type, {x, y, *z}, yaw});
        }
    }
    return placed;
}

} // namespace odyssee::server
