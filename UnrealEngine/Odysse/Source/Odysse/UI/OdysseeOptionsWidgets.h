// Écrans d'options, partagés par le menu principal et le menu pause : touches et paramètres.
// Le bouton Retour est public : l'écran parent décide où il ramène.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeOptionsWidgets.generated.h"

class UComboBoxString;
class UScrollBox;
class USlider;

/** Liste des actions et de leur touche (touches par défaut : docs/GAMEPLAY.md). */
UCLASS()
class ODYSSE_API UOdysseeKeyBindingsWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UScrollBox> BindingList;
	/** KeyButton1, KeyButton2… dans l'ordre de la liste. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TArray<TObjectPtr<UButton>> KeyButtons;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UButton> BackButton;

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildRow(int32 Index, const FText& Action, const FText& Key);
};

/** Souris, son, qualité graphique. */
UCLASS()
class ODYSSE_API UOdysseeSettingsWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<USlider> MouseSensitivitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<USlider> VolumeSlider;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UComboBoxString> GraphicsPresetCombo;
	/** Ouvre l'écran des touches (menu pause ; caché dans le menu principal, qui a son bouton). */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UButton> KeyBindingsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UButton> ApplyButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UButton> BackButton;

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildSettingRow(const FText& Label, UWidget* Control);
	USlider* MakeSlider(FName Name, float Value);
};
