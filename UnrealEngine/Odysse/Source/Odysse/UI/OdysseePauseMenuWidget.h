// Menu pause (Échap, docs/GAMEPLAY.md) : Reprendre, Touches, Paramètres, Enregistrer, Charger,
// Nouvelle partie, Quitter. En coop, Enregistrer, Charger et Nouvelle partie sont grisés.
//
// Navigation vers les options ici ; les actions seront branchées par le jeu sur les boutons.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseePauseMenuWidget.generated.h"

class UOdysseeKeyBindingsWidget;
class UOdysseeSettingsWidget;
class UWidgetSwitcher;

UCLASS()
class ODYSSE_API UOdysseePauseMenuWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Coop : la sauvegarde est automatique et seul l'hôte charge un monde (depuis le menu principal). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|UI")
	void SetCoopMode(bool bCoop);

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> ResumeButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> KeyBindingsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> SettingsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> SaveButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> LoadButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> NewGameButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> QuitButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeKeyBindingsWidget> KeyBindings;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeSettingsWidget> Settings;

	UFUNCTION() void ShowRoot();
	UFUNCTION() void ShowKeyBindings();
	UFUNCTION() void ShowSettings();

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildRootPage();
};
