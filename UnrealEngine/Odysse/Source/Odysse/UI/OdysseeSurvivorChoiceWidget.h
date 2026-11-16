// Choix du survivant (docs/HISTOIRE.md § 4) : le premier écran en entrant dans une partie, après
// « Nouvelle partie » ou « Rejoindre ». Quatre cartes, sans portrait (les personnages ne sont pas
// encore dessinés) : capsule, nom, spécialité. Ceux déjà incarnés dans ce monde sont grisés.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeSurvivorChoiceWidget.generated.h"

/** Ce qu'affiche une carte. */
struct FOdysseeSurvivorView
{
	int32 Index = 0; ///< rang dans data/story.txt
	FText Name;
	FText Capsule;
	FText Specialty;
	FText Skill;
	bool bTaken = false;
};

DECLARE_DELEGATE_OneParam(FOdysseeSurvivorChosen, int32);

/** Une carte de survivant. */
UCLASS()
class ODYSSE_API UOdysseeSurvivorCardWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	void Show(const FOdysseeSurvivorView& View);

	FOdysseeSurvivorChosen OnChosen;

protected:
	virtual UWidget* BuildTree() override;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY() TObjectPtr<UTextBlock> CapsuleText;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> SpecialtyText;
	UPROPERTY() TObjectPtr<UTextBlock> SkillText;
	UPROPERTY() TObjectPtr<UButton> ChooseButton;
	UPROPERTY() TObjectPtr<UTextBlock> ChooseLabel;
	int32 Index = 0;
};

UCLASS()
class ODYSSE_API UOdysseeSurvivorChoiceWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Les survivants (ordre de data/story.txt), ceux déjà incarnés grisés. */
	void ShowSurvivors(const TArray<FOdysseeSurvivorView>& Survivors);
	/** Message sous les cartes (« En attente du serveur… », « Ce survivant vient d'être pris »). */
	void ShowStatus(const FText& Status);

	/** Le joueur choisit un survivant : à transmettre (au serveur en ligne). */
	FOdysseeSurvivorChosen OnSurvivorChosen;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UHorizontalBox> Cards;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> BackButton;

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildBackground();
	void ForwardChoice(int32 Index);
};
