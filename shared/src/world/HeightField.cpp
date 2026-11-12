#include "odyssee/world/WorldFile.hpp"

#include <algorithm>

namespace odyssee::world {

namespace {

constexpr std::int32_t kRawZero = 32768;
constexpr std::int32_t kUnitsPerRawStep = 4; // un pas brut = 1/64 m = 4/256 m

/// Division arrondie vers le bas (la division entière de C++ tronque vers zéro) : une pente
/// négative ne doit pas arrondir autrement qu'une pente positive.
std::int64_t floorDivide(std::int64_t value, std::int64_t divisor) {
    const std::int64_t quotient = value / divisor;
    return (value % divisor != 0 && value < 0) ? quotient - 1 : quotient;
}

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

std::int32_t HeightField::surfaceHeight(std::int32_t x, std::int32_t y) const {
    const std::int64_t size = quadSize;
    const std::int64_t px = std::clamp<std::int64_t>(x, 0, width());
    const std::int64_t py = std::clamp<std::int64_t>(y, 0, depth());
    const std::int64_t col = std::min<std::int64_t>(px / size, std::int64_t{quadsX} - 1);
    const std::int64_t row = std::min<std::int64_t>(py / size, std::int64_t{quadsY} - 1);
    const std::int64_t fx = px - col * size; // 0 à size, dans la case
    const std::int64_t fy = py - row * size;

    const std::int64_t h00 = height(col, row);
    const std::int64_t h11 = height(col + 1, row + 1);
    // Au-dessus de la diagonale (fx ≥ fy) : triangle (0,0) (1,0) (1,1) ; sinon (0,0) (0,1) (1,1).
    const std::int64_t slope =
        fx >= fy ? (height(col + 1, row) - h00) * fx + (h11 - height(col + 1, row)) * fy
                 : (h11 - height(col, row + 1)) * fx + (height(col, row + 1) - h00) * fy;
    return static_cast<std::int32_t>(h00 + floorDivide(slope, size));
}

std::int64_t HeightField::width() const {
    return std::int64_t{quadsX} * quadSize;
}

std::int64_t HeightField::depth() const {
    return std::int64_t{quadsY} * quadSize;
}

bool rowMajorLess(const QuadCoord& a, const QuadCoord& b) {
    return a.row != b.row ? a.row < b.row : a.col < b.col;
}

bool WorldFile::isHole(QuadCoord quad) const {
    return std::binary_search(holes.begin(), holes.end(), quad, rowMajorLess);
}

} // namespace odyssee::world
