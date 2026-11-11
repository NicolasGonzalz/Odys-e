#include "odyssee/world/WorldFile.hpp"

#include <algorithm>

namespace odyssee::world {

namespace {

constexpr std::int32_t kRawZero = 32768;
constexpr std::int32_t kUnitsPerRawStep = 4; // un pas brut = 1/64 m = 4/256 m

} // namespace

std::int32_t heightFromRaw(std::uint16_t raw) {
    return (std::int32_t{raw} - kRawZero) * kUnitsPerRawStep;
}

std::uint16_t rawFromHeight(std::int32_t units) {
    // Arrondi au plus proche, symétrique autour de zéro (division entière tronquée).
    const std::int32_t half = units >= 0 ? kUnitsPerRawStep / 2 : -kUnitsPerRawStep / 2;
    const std::int32_t raw = (units + half) / kUnitsPerRawStep + kRawZero;
    return static_cast<std::uint16_t>(std::clamp(raw, 0, 65535));
}

std::size_t HeightField::samplesPerChunk() const {
    const std::size_t side = std::size_t{chunkQuads} + 1;
    return side * side;
}

std::uint16_t HeightField::raw(std::int64_t col, std::int64_t row) const {
    col = std::clamp<std::int64_t>(col, 0, quadsX);
    row = std::clamp<std::int64_t>(row, 0, quadsY);
    // Un point sur la frontière de deux blocs est dans les deux : on prend le bloc de droite
    // (ou du bas), sauf au bord de la carte.
    const std::int64_t chunkX = std::min<std::int64_t>(col / chunkQuads, chunksX - 1);
    const std::int64_t chunkY = std::min<std::int64_t>(row / chunkQuads, chunksY - 1);
    const std::int64_t localX = col - chunkX * chunkQuads;
    const std::int64_t localY = row - chunkY * chunkQuads;
    const std::size_t chunk = static_cast<std::size_t>(chunkY * chunksX + chunkX);
    const std::size_t offset = static_cast<std::size_t>(localY * (chunkQuads + 1) + localX);
    return samples[chunk * samplesPerChunk() + offset];
}

std::int32_t HeightField::height(std::int64_t col, std::int64_t row) const {
    return heightFromRaw(raw(col, row));
}

bool rowMajorLess(const QuadCoord& a, const QuadCoord& b) {
    return a.row != b.row ? a.row < b.row : a.col < b.col;
}

bool WorldFile::isHole(QuadCoord quad) const {
    return std::binary_search(holes.begin(), holes.end(), quad, rowMajorLess);
}

} // namespace odyssee::world
