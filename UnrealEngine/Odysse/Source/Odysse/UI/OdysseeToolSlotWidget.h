// Une case de la barre d'outils : icône, touche (1 à 5), quantité, usure, état « active ».

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeToolSlotWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class ODYSSE_API UOdysseeToolSlotWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Numéro de la touche affichée (1 à 5). */
	void SetKey(int32 InKey);

	/** Objet de la case : icône (texture d'interface, vide = case vide), quantité (cachée si 1 ou
	 *  moins), usure de 0 à 1 (cachée si négative : objet sans usure). */
	void ShowItem(const TCHAR* InIcon, int32 Count, float Durability);
	void SetActive(bool bActive);

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UImage> Frame;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UImage> Icon;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> CountText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UProgressBar> DurabilityBar;

protected:
	virtual UWidget* BuildTree() override;
};
