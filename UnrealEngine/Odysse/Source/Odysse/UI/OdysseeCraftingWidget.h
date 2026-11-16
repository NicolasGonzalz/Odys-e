// Écran d'artisanat (touche C) : la liste des recettes, le détail de celle choisie (station,
// ingrédients, durée) et le bouton Fabriquer. Le serveur vérifie et répond : fabrication commencée
// (barre de progression), finie, ou refusée (pourquoi).
//
// Le jeu continue derrière (jamais de pause) : l'écran ne fait qu'afficher et transmettre.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeCraftingWidget.generated.h"

class UProgressBar;
class UScrollBox;

/** Ce qu'affiche une recette. */
struct FOdysseeRecipeView
{
	FName Key;
	FText Name;
	FText Station;     ///< « À la main », « À l'établi », « À la forge »
	FText Ingredients; ///< « 1 Bâton, 2 Pierre »
	float Seconds = 0.f;
};

DECLARE_DELEGATE_OneParam(FOdysseeCraftRequested, FName);
DECLARE_DELEGATE_OneParam(FOdysseeRecipeChosen, int32);

/** Une ligne de la liste : un bouton qui sait quelle recette il représente. */
UCLASS()
class ODYSSE_API UOdysseeRecipeEntryWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	void Show(int32 InIndex, const FText& Text);
	void SetSelected(bool bSelected);

	FOdysseeRecipeChosen OnChosen;

protected:
	virtual UWidget* BuildTree() override;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UButton> Button;
	UPROPERTY()
	TObjectPtr<UTextBlock> Label;
	int32 Index = 0;
};

UCLASS()
class ODYSSE_API UOdysseeCraftingWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Remplace la liste des recettes (ordre de data/recipes.txt). */
	void ShowRecipes(const TArray<FOdysseeRecipeView>& Recipes);

	/** Fabrication commencée : la barre se remplit en `Seconds`. */
	void ShowCraftStarted(FName Recipe, float Seconds);
	/** Fin ou refus : message sous le bouton (`bSuccess` : en vert). */
	void ShowCraftMessage(const FText& Message, bool bSuccess);

	/** Le joueur demande à fabriquer la recette choisie : à transmettre au serveur. */
	FOdysseeCraftRequested OnCraftRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UScrollBox> RecipeList;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> RecipeName;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> StationText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> IngredientsText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> DurationText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UButton> CraftButton;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UProgressBar> CraftProgress;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> StatusText;

protected:
	virtual UWidget* BuildTree() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

private:
	UWidget* BuildList();
	UWidget* BuildDetail();
	void Select(int32 Index);
	UFUNCTION()
	void HandleCraftClicked();

	TArray<FOdysseeRecipeView> Shown;
	UPROPERTY()
	TArray<TObjectPtr<UOdysseeRecipeEntryWidget>> Entries;
	int32 Selected = -1;
	float CraftElapsed = 0.f;
	float CraftDuration = 0.f; ///< 0 : pas de fabrication en cours
};
