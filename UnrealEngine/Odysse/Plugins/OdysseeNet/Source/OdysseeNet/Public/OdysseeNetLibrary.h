// Fonctions Blueprint de base d'OdysseeNet : vérifier que shared/ est bien embarqué dans Unreal.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OdysseeNetLibrary.generated.h"

UCLASS()
class ODYSSEENET_API UOdysseeNetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Version de shared/ et du protocole réseau, ex. « shared 0.1.0, protocole 1 ». */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Reseau")
	static FString GetSharedVersion();

	/**
	 * Écrit un nombre et un texte avec ByteWriter (shared/), puis les relit avec ByteReader.
	 * Renvoie vrai si la relecture redonne exactement les mêmes valeurs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Reseau")
	static bool RoundTripThroughByteBuffer(int32 Number, const FString& Text, int32& OutNumber,
		FString& OutText);
};
