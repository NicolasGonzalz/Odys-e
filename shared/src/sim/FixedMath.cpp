#include "odyssee/sim/FixedMath.hpp"

#include <array>

namespace odyssee::sim {

namespace {

/// Quart d'onde du sinus : sin(i × 90° / 256) en Q15, pour i = 0 à 256. Valeurs figées
/// (générées une fois) : aucune fonction de la bibliothèque standard, donc mêmes résultats
/// sur toutes les plateformes et tous les compilateurs.
constexpr std::array<std::int32_t, 257> kQuarterSine{
    0, 201, 402, 603, 804, 1005, 1206, 1407, 1608, 1809, 2009, 2210,
    2411, 2611, 2811, 3012, 3212, 3412, 3612, 3812, 4011, 4211, 4410, 4609,
    4808, 5007, 5205, 5404, 5602, 5800, 5998, 6195, 6393, 6590, 6787, 6983,
    7180, 7376, 7571, 7767, 7962, 8157, 8351, 8546, 8740, 8933, 9127, 9319,
    9512, 9704, 9896, 10088, 10279, 10469, 10660, 10850, 11039, 11228, 11417, 11605,
    11793, 11980, 12167, 12354, 12540, 12725, 12910, 13095, 13279, 13463, 13646, 13828,
    14010, 14192, 14373, 14553, 14733, 14912, 15091, 15269, 15447, 15624, 15800, 15976,
    16151, 16326, 16500, 16673, 16846, 17018, 17190, 17361, 17531, 17700, 17869, 18037,
    18205, 18372, 18538, 18703, 18868, 19032, 19195, 19358, 19520, 19681, 19841, 20001,
    20160, 20318, 20475, 20632, 20788, 20943, 21097, 21251, 21403, 21555, 21706, 21856,
    22006, 22154, 22302, 22449, 22595, 22740, 22884, 23028, 23170, 23312, 23453, 23593,
    23732, 23870, 24008, 24144, 24279, 24414, 24548, 24680, 24812, 24943, 25073, 25202,
    25330, 25457, 25583, 25708, 25833, 25956, 26078, 26199, 26320, 26439, 26557, 26674,
    26791, 26906, 27020, 27133, 27246, 27357, 27467, 27576, 27684, 27791, 27897, 28002,
    28106, 28209, 28311, 28411, 28511, 28610, 28707, 28803, 28899, 28993, 29086, 29178,
    29269, 29359, 29448, 29535, 29622, 29707, 29792, 29875, 29957, 30038, 30118, 30196,
    30274, 30350, 30425, 30499, 30572, 30644, 30715, 30784, 30853, 30920, 30986, 31050,
    31114, 31177, 31238, 31298, 31357, 31415, 31471, 31527, 31581, 31634, 31686, 31737,
    31786, 31834, 31881, 31927, 31972, 32015, 32058, 32099, 32138, 32177, 32214, 32251,
    32286, 32319, 32352, 32383, 32413, 32442, 32470, 32496, 32522, 32546, 32568, 32590,
    32610, 32629, 32647, 32664, 32679, 32693, 32706, 32718, 32729, 32738, 32746, 32753,
    32758, 32762, 32766, 32767, 32768,
};

constexpr std::int32_t kStepsPerTurn = 1024;   ///< 256 pas par quart de tour
constexpr std::int32_t kYawUnitsPerStep = 64;  ///< 65536 / 1024

/// Sinus d'un pas entier (0 à 1023, un tour), par symétries du quart d'onde.
std::int32_t sineOfStep(std::int32_t step) {
    step %= kStepsPerTurn;
    const std::int32_t quadrant = step / 256;
    const std::int32_t index = step % 256;
    const std::int32_t mirrored = (quadrant % 2 == 0) ? index : 256 - index;
    const std::int32_t value = kQuarterSine[static_cast<std::size_t>(mirrored)];
    return quadrant < 2 ? value : -value;
}

} // namespace

std::int32_t sinQ15(std::uint16_t yaw) {
    // Interpolation linéaire entre deux pas de la table (64 unités de cap par pas).
    const std::int32_t step = yaw / kYawUnitsPerStep;
    const std::int32_t fraction = yaw % kYawUnitsPerStep;
    const std::int32_t a = sineOfStep(step);
    const std::int32_t b = sineOfStep(step + 1);
    return a + (b - a) * fraction / kYawUnitsPerStep;
}

std::int32_t cosQ15(std::uint16_t yaw) {
    return sinQ15(static_cast<std::uint16_t>(yaw + 16384)); // cos(a) = sin(a + 90°)
}

std::uint32_t isqrt(std::uint64_t value) {
    // Bit à bit, du poids fort au poids faible : exact et sans flottant.
    std::uint64_t result = 0;
    std::uint64_t bit = std::uint64_t{1} << 62;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return static_cast<std::uint32_t>(result);
}

} // namespace odyssee::sim
