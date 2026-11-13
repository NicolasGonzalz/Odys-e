// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OdysseeNetSubsystem.h"
#include "OdyssePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UOdysseeHudWidget;
class UOdysseeMainMenuWidget;
class UOdysseePauseMenuWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 *
 *  Écrans du jeu (T027) : menu principal au lancement (rejoindre un serveur, ou jouer hors ligne),
 *  HUD en partie, menu pause sur Échap. Le HUD est rempli à chaque image.
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
	void TogglePause();

	UPROPERTY(Transient) TObjectPtr<UOdysseeHudWidget> Hud;
	UPROPERTY(Transient) TObjectPtr<UOdysseeMainMenuWidget> MainMenu;
	UPROPERTY(Transient) TObjectPtr<UOdysseePauseMenuWidget> PauseMenu;
	bool bInGame = false;
	TArray<FString> ShownToolbar;     ///< contenu affiché de chaque case (redessin si changement)
	TMap<FString, FString> IconCache; ///< nom de base → texture trouvée (vide : aucune)

};
