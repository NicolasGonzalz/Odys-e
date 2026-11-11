#pragma once

#include <cstdint>

namespace odyssee::net {

/// Quantification : ce qui voyage sur le réseau est en entiers de taille fixe, pas en flottants.
///
/// Le client et le serveur simulent à partir des valeurs **quantifiées** (le client quantifie
/// son input avant de l'appliquer) : même entrée des deux côtés, donc même résultat. Les valeurs
/// invalides (NaN, hors bornes) sont ramenées dans l'intervalle permis, jamais transmises telles
/// quelles.

/// Positions : virgule fixe à 1/256 m (≈ 4 mm), sur 32 bits signés (± 8 000 km).
inline constexpr float kPositionUnitsPerMeter = 256.0f;
[[nodiscard]] std::int32_t quantizePosition(float meters);
[[nodiscard]] float dequantizePosition(std::int32_t units);

/// Cap (yaw), en degrés : un tour complet sur 16 bits (≈ 0,0055°), rebouclé (370° = 10°).
[[nodiscard]] std::uint16_t quantizeYaw(float degrees);
[[nodiscard]] float dequantizeYaw(std::uint16_t units);

/// Inclinaison (pitch), en degrés : de -90° (bas) à +90° (haut), de -32767 à 32767.
inline constexpr std::int16_t kMaxPitchUnits = 32767;
[[nodiscard]] std::int16_t quantizePitch(float degrees);
[[nodiscard]] float dequantizePitch(std::int16_t units);

/// Axe de déplacement (clavier ou manette) : de -1 à 1, de -127 à 127.
inline constexpr std::int8_t kMaxAxisUnits = 127;
[[nodiscard]] std::int8_t quantizeAxis(float value);
[[nodiscard]] float dequantizeAxis(std::int8_t units);

} // namespace odyssee::net
