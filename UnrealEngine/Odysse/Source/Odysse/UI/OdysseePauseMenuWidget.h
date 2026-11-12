// Menu pause (Échap, docs/GAMEPLAY.md) : Reprendre, Paramètres (touches comprises), Enregistrer,
// Quitter. En coop, Enregistrer est grisé (le serveur sauvegarde tout seul).
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
	/** Coop : la sauvegarde est automatique (par le serveur). */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|UI")
	void SetCoopMode(bool bCoop);

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> ResumeButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> SettingsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> SaveButton;
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
