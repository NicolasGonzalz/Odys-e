#pragma once

#include <cstdint>

namespace odyssee::sim {

/// Sol provisoire, en attendant que le serveur charge la carte (T041) : plat, à la hauteur du
/// point d'apparition (salle des capsules du centre de stase, terrain à 8,55 m et sol du bunker
/// 30 cm au-dessus).
///
/// Partagé par le serveur et le client : la prédiction doit marcher exactement sur le même sol
/// que la simulation du serveur, sinon les deux divergent dès le premier tick.
inline constexpr float kProvisionalGroundMeters = 8.85f;

/// Hauteur du sol provisoire, en 1/256 m (pour sim::FlatTerrain).
[[nodiscard]] std::int32_t provisionalGroundHeight();

} // namespace odyssee::sim
