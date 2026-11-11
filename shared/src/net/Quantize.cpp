#include "odyssee/net/Quantize.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace odyssee::net {

namespace {

constexpr double kFullTurnUnits = 65536.0;
constexpr double kMaxPitchDegrees = 90.0;

/// Arrondit `value` à l'entier le plus proche dans [low, high] ; NaN donne 0.
template <typename Int> Int roundClamped(double value, double low, double high) {
    if (std::isnan(value)) {
        return 0;
    }
    return static_cast<Int>(std::lround(std::clamp(value, low, high)));
}

} // namespace

std::int32_t quantizePosition(float meters) {
    constexpr double low = std::numeric_limits<std::int32_t>::min();
    constexpr double high = std::numeric_limits<std::int32_t>::max();
    // std::llround : la position en unités peut dépasser un long 32 bits avant le clamp (Windows).
    const double units = static_cast<double>(meters) * kPositionUnitsPerMeter;
    if (std::isnan(units)) {
        return 0;
    }
    return static_cast<std::int32_t>(std::llround(std::clamp(units, low, high)));
}

float dequantizePosition(std::int32_t units) {
    return static_cast<float>(static_cast<double>(units) / kPositionUnitsPerMeter);
}

std::uint16_t quantizeYaw(float degrees) {
    if (!std::isfinite(degrees)) {
        return 0;
    }
    double turns = std::fmod(static_cast<double>(degrees), 360.0) / 360.0;
    if (turns < 0.0) {
        turns += 1.0;
    }
    // 359,999° arrondit à 65536 : le masque le ramène à 0 (un tour complet).
    return static_cast<std::uint16_t>(std::llround(turns * kFullTurnUnits) & 0xFFFF);
}

float dequantizeYaw(std::uint16_t units) {
    return static_cast<float>(static_cast<double>(units) * 360.0 / kFullTurnUnits);
}

std::int16_t quantizePitch(float degrees) {
    const double units = static_cast<double>(degrees) / kMaxPitchDegrees * kMaxPitchUnits;
    return roundClamped<std::int16_t>(units, -kMaxPitchUnits, kMaxPitchUnits);
}

float dequantizePitch(std::int16_t units) {
    return static_cast<float>(static_cast<double>(units) * kMaxPitchDegrees / kMaxPitchUnits);
}

std::int8_t quantizeAxis(float value) {
    const double units = static_cast<double>(value) * kMaxAxisUnits;
    return roundClamped<std::int8_t>(units, -kMaxAxisUnits, kMaxAxisUnits);
}

float dequantizeAxis(std::int8_t units) {
    return static_cast<float>(static_cast<double>(units) / kMaxAxisUnits);
}

} // namespace odyssee::net
