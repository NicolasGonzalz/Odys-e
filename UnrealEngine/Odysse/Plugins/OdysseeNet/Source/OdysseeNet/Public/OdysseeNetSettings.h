// Réglages du plugin, dans Paramètres du projet → Odyssee → Réseau (Config/DefaultGame.ini).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "OdysseeNetSettings.generated.h"

class AOdysseeRemotePlayer;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Réseau"))
class ODYSSEENET_API UOdysseeNetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UOdysseeNetSettings();

	/** Acteur qui montre les autres joueurs (un Blueprint enfant d'AOdysseeRemotePlayer). */
	UPROPERTY(Config, EditAnywhere, Category = "Joueurs")
	TSoftClassPtr<AOdysseeRemotePlayer> RemotePlayerClass;

	virtual FName GetCategoryName() const override { return TEXT("Odyssee"); }
};
