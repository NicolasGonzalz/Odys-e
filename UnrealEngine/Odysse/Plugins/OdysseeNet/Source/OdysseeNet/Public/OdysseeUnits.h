// Passage entre les unités de la simulation (shared/) et celles d'Unreal (ADR 0003, règle 2).
//
// Mêmes axes des deux côtés : X vers l'est, Y vers le sud, Z vers le haut (repère main gauche,
// comme celui d'Unreal), et le cap tourne de +X vers +Y. Seule l'échelle change : la simulation
// compte en 1/256 m, Unreal en centimètres.
#pragma once

#include "CoreMinimal.h"

#include <array>
#include <cstdint>

namespace OdysseeUnits
{
	/** Centimètres Unreal par unité de simulation (1/256 m). */
	inline constexpr double CentimetersPerUnit = 100.0 / 256.0;

	/** Position de simulation (1/256 m) → point Unreal (cm). */
	ODYSSEENET_API FVector ToUnreal(const std::array<std::int32_t, 3>& Position);

	/** Point Unreal (cm) → position de simulation (1/256 m), arrondie. */
	ODYSSEENET_API std::array<std::int32_t, 3> FromUnreal(const FVector& Location);

	/** Cap de simulation (un tour = 65536) → degrés Unreal, et l'inverse. */
	ODYSSEENET_API float YawToDegrees(std::uint16_t Yaw);
	ODYSSEENET_API std::uint16_t YawFromDegrees(float Degrees);

	/** Inclinaison du regard en degrés Unreal (positive vers le haut) → simulation. */
	ODYSSEENET_API std::int16_t PitchFromDegrees(float Degrees);
}
