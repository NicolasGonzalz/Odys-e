#include "OdysseeWorldSubsystem.h"

#include "Misc/Paths.h"

#include "odyssee/sim/ProvisionalGround.hpp"
#include "odyssee/world/WorldFileLoader.hpp"

#include <string>
#include <utility>

DEFINE_LOG_CATEGORY_STATIC(LogOdysseeWorld, Log, All);

void UOdysseeWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Flat = MakeUnique<odyssee::sim::FlatTerrain>(odyssee::sim::provisionalGroundHeight());
	const FString Path = FindMapFile();
	if (Path.IsEmpty())
	{
		UE_LOG(LogOdysseeWorld, Warning,
			TEXT("Fichier monde introuvable (git lfs pull ?) : sol plat provisoire."));
		return;
	}
	LoadMap(Path);
}

const odyssee::sim::Terrain& UOdysseeWorldSubsystem::GetTerrain() const
{
	if (Map.IsValid())
	{
		return *Map;
	}
	return *Flat;
}

FString UOdysseeWorldSubsystem::FindMapFile()
{
	const FString Candidates[] = {
		FPaths::Combine(FPaths::ProjectDir(), TEXT("World"), TEXT("odyssee.world")),
		FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), TEXT(".."), TEXT("tools"), TEXT("worldgen"),
			TEXT("output"), TEXT("odyssee.world")),
	};
	for (const FString& Candidate : Candidates)
	{
		const FString Full = FPaths::ConvertRelativePathToFull(Candidate);
		if (FPaths::FileExists(Full))
		{
			return Full;
		}
	}
	return FString();
}

void UOdysseeWorldSubsystem::LoadMap(const FString& Path)
{
	auto Decoded = odyssee::world::loadWorldFile(std::string(TCHAR_TO_UTF8(*Path)));
	if (!Decoded.world)
	{
		UE_LOG(LogOdysseeWorld, Error, TEXT("Carte « %s » non chargée : %s. Sol plat provisoire."),
			*Path, UTF8_TO_TCHAR(odyssee::world::describe(Decoded.error)));
		return;
	}
	Map = MakeUnique<odyssee::world::WorldTerrain>(std::move(*Decoded.world));
	MapPath = Path;
	UE_LOG(LogOdysseeWorld, Log, TEXT("Carte « %s » chargée : %d volumes de collision."), *Path,
		static_cast<int32>(Map->world().boxes.size()));
}
