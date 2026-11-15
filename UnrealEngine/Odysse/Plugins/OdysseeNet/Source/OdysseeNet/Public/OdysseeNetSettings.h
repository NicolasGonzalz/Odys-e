// Réglages du plugin, dans Paramètres du projet → Odyssee → Réseau (Config/DefaultGame.ini).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "OdysseeNetSettings.generated.h"

class AOdysseeBagActor;
class AOdysseeRemotePlayer;
class AOdysseeResourceActor;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Réseau"))
class ODYSSEENET_API UOdysseeNetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UOdysseeNetSettings();

	/** Acteur qui montre les autres joueurs (un Blueprint enfant d'AOdysseeRemotePlayer). */
	UPROPERTY(Config, EditAnywhere, Category = "Joueurs")
	TSoftClassPtr<AOdysseeRemotePlayer> RemotePlayerClass;

	/** Acteur qui montre les ressources récoltables (AOdysseeResourceActor ou un Blueprint enfant,
	 *  pour remplacer les formes provisoires par des modèles). */
	UPROPERTY(Config, EditAnywhere, Category = "Ressources")
	TSoftClassPtr<AOdysseeResourceActor> ResourceClass;

	/** Acteur qui montre les sacs laissés à la mort (AOdysseeBagActor ou un Blueprint enfant). */
	UPROPERTY(Config, EditAnywhere, Category = "Sacs")
	TSoftClassPtr<AOdysseeBagActor> BagClass;

	virtual FName GetCategoryName() const override { return TEXT("Odyssee"); }
};
