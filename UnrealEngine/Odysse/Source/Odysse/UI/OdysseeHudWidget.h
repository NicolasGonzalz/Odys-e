// HUD en jeu : santé et endurance (barres), faim et soif (jauges rondes) en bas à gauche, barre
// d'outils de 5 cases en bas au centre, notifications en haut à droite, infos réseau en haut à
// gauche, réticule au centre.
//
// Pas encore branché sur le jeu : ShowPreviewValues affiche des valeurs de démonstration. Le C++
// du jeu (T027, T051) remplira ces mêmes éléments.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeHudWidget.generated.h"

class UOdysseeGaugeWidget;
class UOdysseeToolSlotWidget;

UCLASS()
class ODYSSE_API UOdysseeHudWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Valeurs en dur pour juger le rendu (barres à 0,7, ping « 42 ms »…). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|UI")
	void ShowPreviewValues();

	/** Ajoute un message en haut à droite (3 au plus : le plus ancien part). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|UI")
	void AddNotification(const FText& Message);

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UOdysseeGaugeWidget> HungerGauge;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UOdysseeGaugeWidget> ThirstGauge;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TArray<TObjectPtr<UOdysseeToolSlotWidget>> ToolSlots;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> ItemNameText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UVerticalBox> NotificationBox;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> PingText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> ConnectionText;

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildVitals();
	UWidget* BuildVitalBar(const TCHAR* Icon, const FLinearColor& Color, TObjectPtr<UProgressBar>& OutBar,
		FName Name);
	UWidget* BuildToolBar();
	UWidget* BuildNetwork();
	UWidget* BuildCrosshair();
};
