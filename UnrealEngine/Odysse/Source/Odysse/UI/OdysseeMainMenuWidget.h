// Menu principal (docs/MODES.md) : Solo (mondes locaux), Multijoueur (code, favoris, héberger),
// Options (touches, paramètres), Quitter. Fond : une ambiance de la plaine.
//
// La navigation entre les pages se fait ici (c'est de l'interface) ; les actions (lancer une
// partie, rejoindre, quitter) seront branchées par le jeu sur les boutons publics.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeMainMenuWidget.generated.h"

class UEditableTextBox;
class UOdysseeKeyBindingsWidget;
class UOdysseeSettingsWidget;
class UScrollBox;
class UWidgetSwitcher;

UCLASS()
class ODYSSE_API UOdysseeMainMenuWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> SoloButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> MultiplayerButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> OptionsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> QuitButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UScrollBox> WorldList;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> NewWorldButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> LoadWorldButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UEditableTextBox> AddressInput;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UEditableTextBox> NameInput;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UEditableTextBox> JoinCodeInput;
	/** Où en est la connexion : « Connexion… », refus, serveur muet… */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> JoinStatusText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> JoinButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UScrollBox> FavoritesList;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> HostButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeKeyBindingsWidget> KeyBindings;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeSettingsWidget> Settings;

	UFUNCTION() void ShowRoot();
	UFUNCTION() void ShowSolo();
	UFUNCTION() void ShowMultiplayer();
	UFUNCTION() void ShowOptions();
	UFUNCTION() void ShowKeyBindings();
	UFUNCTION() void ShowSettings();

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildBackground();
	UWidget* BuildTitle();
	UWidget* BuildRootPage();
	UWidget* BuildSoloPage();
	UWidget* BuildMultiplayerPage();
	UWidget* BuildOptionsPage();
	UWidget* BuildJoinRow();
	UEditableTextBox* MakeField(FName Name, const FText& Hint);
	/** Un bouton Retour qui ramène à `Page` (une des fonctions Show…). */
	UButton* MakeBackButton(FName Target);
};
