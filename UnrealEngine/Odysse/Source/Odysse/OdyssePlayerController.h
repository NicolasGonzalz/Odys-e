// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OdysseeNetSubsystem.h"
#include "UI/OdysseeBraceletWidget.h"
#include "OdyssePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UOdysseeBraceletWidget;
class UOdysseeCraftingWidget;
class UOdysseeHudWidget;
class UOdysseeInventoryWidget;
class UOdysseeMainMenuWidget;
class UOdysseePauseMenuWidget;
class UOdysseeSurvivorChoiceWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 *
 *  Écrans du jeu (T027) : menu principal au lancement (rejoindre un serveur, ou jouer hors ligne),
 *  HUD en partie, menu pause sur Échap. Le HUD est rempli à chaque image. Inventaire (Tab) et
 *  artisanat (C) : des panneaux par-dessus le jeu, qui ne le mettent jamais en pause
 *  (OdyssePlayerControllerPanels.cpp).
 */
UCLASS(abstract)
class ODYSSE_API AOdyssePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AOdyssePlayerController();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	virtual void PlayerTick(float DeltaTime) override;

private:
	void CreateScreens();
	void ShowMenu(const FText& Status);
	void ShowGame();
	void UpdateHud();
	void UpdateToolbar(const TArray<FOdysseeInventorySlot>& Inventory);
	FString IconFor(const FString& BaseName);
	void SetMenuInput(UUserWidget* Focus);
	void SetGameInput();
	UOdysseeNetSubsystem* GetNet() const;

	UFUNCTION() void OnJoinClicked();
	UFUNCTION() void OnPlayOfflineClicked();
	UFUNCTION() void OnQuitGameClicked();
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnLeaveClicked();
	UFUNCTION() void OnConnectionChanged(EOdysseeConnection State, const FText& Status);
	UFUNCTION() void OnPlayerDied(bool bLeftBag);

	// Choix du survivant (OdyssePlayerControllerSurvivor.cpp).
	void CreateSurvivorChoice();
	void ShowSurvivorChoice(const FText& Status);
	void RefreshSurvivorChoice();
	void OnSurvivorPicked(int32 Index);
	UFUNCTION() void OnSurvivorsChanged();
	UFUNCTION() void OnSurvivorChoiceBack();
	void TogglePause();

	// Panneaux inventaire et artisanat (OdyssePlayerControllerPanels.cpp).
	void CreatePanels();
	UFUNCTION() void ToggleBracelet();
	UFUNCTION() void OpenBagTab();
	UFUNCTION() void ToggleCrafting();
	UFUNCTION() void OpenStoryTab();
	UFUNCTION() void OpenMapTab();
	void ToggleTab(EOdysseeBraceletTab Tab);
	bool CanUseBracelet() const;
	void OpenBracelet(EOdysseeBraceletTab Tab);
	void FillCrafting(FName Station);
	/** Artisanat d'une station : « hand » (touche C : à la main seulement), « workbench » ou « forge »
	 *  (leurs recettes, plus celles à la main). */
	void ToggleStation(FName Station);
	/** Console, en attendant que les établis et forges soient posés dans le monde (on les ouvrira
	 *  en interagissant avec eux) : « OdysseeStation etabli », « OdysseeStation forge ». */
	UFUNCTION(Exec) void OdysseeStation(const FString& Which);
	/** Ferme l'inventaire ou l'artisanat ; vrai s'il y en avait un d'ouvert. */
	bool ClosePanels();
	void UpdateInventoryPanel();
	void UpdateCraftingPanel();
	void RequestMove(int32 From, int32 To);
	void RequestCraft(FName Recipe);
	UFUNCTION() void OnCraftChanged(FName RecipeKey, EOdysseeCraftOutcome Outcome, float Seconds);

	UPROPERTY(Transient) TObjectPtr<UOdysseeHudWidget> Hud;
	UPROPERTY(Transient) TObjectPtr<UOdysseeMainMenuWidget> MainMenu;
	UPROPERTY(Transient) TObjectPtr<UOdysseePauseMenuWidget> PauseMenu;
	UPROPERTY(Transient) TObjectPtr<UOdysseeSurvivorChoiceWidget> SurvivorChoice;
	int32 PendingSurvivor = -1; ///< choix envoyé au serveur, en attente de sa réponse
	UPROPERTY(Transient) TObjectPtr<UOdysseeBraceletWidget> Bracelet;
	EOdysseeBraceletTab LastTab = EOdysseeBraceletTab::Bag;
	FName OpenStationName; ///< station dont l'artisanat est ouvert
	bool bInGame = false;
	TArray<FString> ShownToolbar;     ///< contenu affiché de chaque case (redessin si changement)
	TMap<FString, FString> IconCache; ///< nom de base → texture trouvée (vide : aucune)
	int32 ShownHeldSlot = -1;         ///< case tenue affichée (redessin si changement)

};
