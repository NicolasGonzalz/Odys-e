#include "odyssee/sim/FixedMath.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <numbers>

using namespace odyssee::sim;

TEST_CASE("FixedMath : sinus et cosinus aux angles remarquables") {
    CHECK(sinQ15(0) == 0);
    CHECK(cosQ15(0) == kQ15One);
    CHECK(sinQ15(16384) == kQ15One); // 90°
    CHECK(cosQ15(16384) == 0);
    CHECK(sinQ15(32768) == 0); // 180°
    CHECK(cosQ15(32768) == -kQ15One);
    CHECK(sinQ15(49152) == -kQ15One); // 270°
}

TEST_CASE("FixedMath : sinus et cosinus proches des vrais, sur tout le tour") {
    // std::sin est permis ici : il sert de référence au test, jamais à la simulation.
    for (std::uint32_t yaw = 0; yaw < 65536; yaw += 97) {
        const double radians = yaw * 2.0 * std::numbers::pi / 65536.0;
        const auto units = static_cast<std::uint16_t>(yaw);
        CAPTURE(yaw);
        CHECK(std::abs(sinQ15(units) / 32768.0 - std::sin(radians)) < 5e-4);
        CHECK(std::abs(cosQ15(units) / 32768.0 - std::cos(radians)) < 5e-4);
    }
}

TEST_CASE("FixedMath : racine carrée entière") {
    CHECK(isqrt(0) == 0);
    CHECK(isqrt(1) == 1);
    CHECK(isqrt(15) == 3);
    CHECK(isqrt(16) == 4);
    CHECK(isqrt(17) == 4);
    CHECK(isqrt(4'000'000'000'000ULL) == 2'000'000);
    for (std::uint64_t n = 0; n < 5000; n += 7) {
        const std::uint64_t root = isqrt(n);
        CHECK(root * root <= n);
        CHECK((root + 1) * (root + 1) > n);
    }
}
