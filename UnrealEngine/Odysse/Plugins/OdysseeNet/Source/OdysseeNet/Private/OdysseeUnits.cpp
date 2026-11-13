#include "OdysseeUnits.h"

#include "odyssee/net/Quantize.hpp"

#include <cmath>

namespace OdysseeUnits
{
	FVector ToUnreal(const std::array<std::int32_t, 3>& Position)
	{
		return FVector(Position[0] * CentimetersPerUnit, Position[1] * CentimetersPerUnit,
			Position[2] * CentimetersPerUnit);
	}

	std::array<std::int32_t, 3> FromUnreal(const FVector& Location)
	{
		const auto Convert = [](double Centimeters)
		{
			return static_cast<std::int32_t>(std::lround(Centimeters / CentimetersPerUnit));
		};
		return {Convert(Location.X), Convert(Location.Y), Convert(Location.Z)};
	}

	float YawToDegrees(std::uint16_t Yaw)
	{
		return odyssee::net::dequantizeYaw(Yaw);
	}

	std::uint16_t YawFromDegrees(float Degrees)
	{
		return odyssee::net::quantizeYaw(static_cast<float>(FRotator::ClampAxis(Degrees)));
	}

	std::int16_t PitchFromDegrees(float Degrees)
	{
		return odyssee::net::quantizePitch(static_cast<float>(FRotator::NormalizeAxis(Degrees)));
	}
}
