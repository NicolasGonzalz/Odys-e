#include "odyssee/net/Quantize.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <limits>

using namespace odyssee::net;

namespace {

constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInfinity = std::numeric_limits<float>::infinity();

} // namespace

TEST_CASE("Quantize : positions au demi-pas près (1/256 m)") {
    constexpr std::array<float, 6> samples{0.0f, 1.0f, -3.5f, 1900.123f, 4032.0f, -100.25f};
    for (const float meters : samples) {
        CAPTURE(meters);
        const float back = dequantizePosition(quantizePosition(meters));
        CHECK(std::abs(back - meters) <= 0.5f / kPositionUnitsPerMeter + 1e-4f);
    }
    CHECK(quantizePosition(1.0f) == 256);
    CHECK(quantizePosition(-1.0f) == -256);
}

TEST_CASE("Quantize : positions invalides ramenées dans les bornes") {
    CHECK(quantizePosition(kNaN) == 0);
    CHECK(quantizePosition(kInfinity) == std::numeric_limits<std::int32_t>::max());
    CHECK(quantizePosition(-kInfinity) == std::numeric_limits<std::int32_t>::min());
    CHECK(quantizePosition(1e12f) == std::numeric_limits<std::int32_t>::max());
}

TEST_CASE("Quantize : le cap reboucle sur un tour") {
    CHECK(quantizeYaw(0.0f) == 0);
    CHECK(quantizeYaw(90.0f) == 16384);
    CHECK(quantizeYaw(180.0f) == 32768);
    CHECK(quantizeYaw(360.0f) == 0);
    CHECK(quantizeYaw(370.0f) == quantizeYaw(10.0f));
    CHECK(quantizeYaw(-90.0f) == quantizeYaw(270.0f));
    CHECK(quantizeYaw(359.9999f) == 0); // arrondi au tour complet, pas 65536
    CHECK(quantizeYaw(kNaN) == 0);
    CHECK(quantizeYaw(kInfinity) == 0);
    CHECK(dequantizeYaw(quantizeYaw(123.4f)) == doctest::Approx(123.4f).epsilon(1e-4));
}

TEST_CASE("Quantize : l'inclinaison est bornée à ±90°") {
    CHECK(quantizePitch(0.0f) == 0);
    CHECK(quantizePitch(90.0f) == kMaxPitchUnits);
    CHECK(quantizePitch(-90.0f) == -kMaxPitchUnits);
    CHECK(quantizePitch(400.0f) == kMaxPitchUnits);
    CHECK(quantizePitch(-400.0f) == -kMaxPitchUnits);
    CHECK(quantizePitch(kNaN) == 0);
    CHECK(dequantizePitch(quantizePitch(-37.5f)) == doctest::Approx(-37.5f).epsilon(1e-3));
}

TEST_CASE("Quantize : axes de déplacement de -127 à 127") {
    CHECK(quantizeAxis(1.0f) == kMaxAxisUnits);
    CHECK(quantizeAxis(-1.0f) == -kMaxAxisUnits);
    CHECK(quantizeAxis(0.5f) == 64); // 63,5 arrondi loin de zéro
    CHECK(quantizeAxis(3.0f) == kMaxAxisUnits);
    CHECK(quantizeAxis(-3.0f) == -kMaxAxisUnits); // jamais -128
    CHECK(quantizeAxis(kNaN) == 0);
    CHECK(dequantizeAxis(kMaxAxisUnits) == doctest::Approx(1.0f));
}

TEST_CASE("Quantize : requantifier une valeur déjà quantifiée ne la change pas") {
    // Le client simule avec l'input quantifié : relire puis requantifier doit être stable.
    for (int value = -kMaxPitchUnits; value <= kMaxPitchUnits; value += 997) {
        const auto pitch = static_cast<std::int16_t>(value);
        CHECK(quantizePitch(dequantizePitch(pitch)) == pitch);
    }
    for (int axis = -kMaxAxisUnits; axis <= kMaxAxisUnits; ++axis) {
        const auto units = static_cast<std::int8_t>(axis);
        CHECK(quantizeAxis(dequantizeAxis(units)) == units);
    }
    for (unsigned yaw = 0; yaw < 65536; yaw += 4099) {
        const auto units = static_cast<std::uint16_t>(yaw);
        CHECK(quantizeYaw(dequantizeYaw(units)) == units);
    }
}
