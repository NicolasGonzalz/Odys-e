#pragma once

#include <cstdint>

namespace odyssee::sim {

/// Calculs entiers de la simulation. Pas de flottant : un même calcul doit donner le même
/// résultat sur le serveur (Linux, GCC) et sur le client (Windows, MSVC dans Unreal), au bit
/// près, sinon la prédiction du client diverge de la vérité du serveur.

/// 1,0 en virgule fixe Q15.
inline constexpr std::int32_t kQ15One = 32768;

/// Sinus et cosinus d'un cap quantifié (net::quantizeYaw : un tour = 65536), en Q15.
/// Précision : ~0,0002 (table de 1024 pas par tour, interpolée).
[[nodiscard]] std::int32_t sinQ15(std::uint16_t yaw);
[[nodiscard]] std::int32_t cosQ15(std::uint16_t yaw);

/// Racine carrée entière (arrondie vers le bas).
[[nodiscard]] std::uint32_t isqrt(std::uint64_t value);

} // namespace odyssee::sim
