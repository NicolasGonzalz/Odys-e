#include "OdysseeCatalogSubsystem.h"

#include "Misc/Paths.h"

#include <string>
#include <utility>

DEFINE_LOG_CATEGORY_STATIC(LogOdysseeCatalog, Log, All);

void UOdysseeCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const FString Dir = FindDataDir();
	if (Dir.IsEmpty())
	{
		UE_LOG(LogOdysseeCatalog, Warning, TEXT("Dossier data/ introuvable : objets sans nom."));
		return;
	}
	LoadItems(Dir);
	LoadResourceTypes(Dir);
}

void UOdysseeCatalogSubsystem::LoadItems(const FString& Dir)
{
	auto Result = odyssee::data::Catalog::fromDirectory(std::string(TCHAR_TO_UTF8(*Dir)));
	if (!Result.catalog)
	{
		UE_LOG(LogOdysseeCatalog, Error, TEXT("Objets non chargés : %s"),
			UTF8_TO_TCHAR(Result.error ? Result.error->describe().c_str() : "erreur inconnue"));
		return;
	}
	Catalog = std::move(Result.catalog);
	UE_LOG(LogOdysseeCatalog, Log, TEXT("Objets chargés depuis %s : %d objets, %d recettes"), *Dir,
		static_cast<int32>(Catalog->items().size()), static_cast<int32>(Catalog->recipes().size()));
}

void UOdysseeCatalogSubsystem::LoadResourceTypes(const FString& Dir)
{
	if (!Catalog)
	{
		return; // les ressources se vérifient contre les objets (objet récolté, outils)
	}
	auto Result = odyssee::data::ResourceCatalog::fromDirectory(std::string(TCHAR_TO_UTF8(*Dir)), *Catalog);
	if (!Result.catalog)
	{
		UE_LOG(LogOdysseeCatalog, Error, TEXT("Ressources non chargées : %s"),
			UTF8_TO_TCHAR(Result.error ? Result.error->describe().c_str() : "erreur inconnue"));
		return;
	}
	ResourceTypes = std::move(Result.catalog);
	UE_LOG(LogOdysseeCatalog, Log, TEXT("Ressources chargées : %d types"),
		static_cast<int32>(ResourceTypes->types().size()));
}

FString UOdysseeCatalogSubsystem::FindDataDir()
{
	const FString Candidates[] = {
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Data")),
		FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), TEXT(".."), TEXT("data")),
	};
	for (const FString& Candidate : Candidates)
	{
		const FString Full = FPaths::ConvertRelativePathToFull(Candidate);
		if (FPaths::FileExists(FPaths::Combine(Full, TEXT("items.txt"))))
		{
			return Full;
		}
	}
	return FString();
}
