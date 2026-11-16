// Écran d'artisanat (touche C) : à gauche, les recettes en grille d'images, rangées par catégorie
// d'objet (Outils, Matériaux, Stations…) ; à droite, la recette choisie en images :
// [bûche] ×8 + [pierre] ×6 → [établi], avec sous chaque ingrédient ce qu'on possède (vert si assez,
// rouge sinon), puis le bouton Fabriquer. Le serveur vérifie et répond : fabrication commencée
// (barre de progression), finie, ou refusée (pourquoi).
//
// Le jeu continue derrière (jamais de pause) : l'écran ne fait qu'afficher et transmettre.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeCraftingWidget.generated.h"

class UHorizontalBox;
class UWrapBox;
class UImage;
class UProgressBar;
class UUniformGridPanel;

/** Un objet d'une recette : icône (texture d'interface, vide si aucune), nom, quantité. */
struct FOdysseeItemView
{
	FName Key;
	FString Icon;
	FText Name;
	int32 Count = 0;
};

/** Ce qu'affiche une recette. */
struct FOdysseeRecipeView
{
	FName Key;
	FName Category;        ///< « tool », « resource »… : un onglet par catégorie
	FText CategoryLabel;   ///< « Outils », « Matériaux »…
	FText Station;         ///< « À la main », « À l'établi », « À la forge »
	float Seconds = 0.f;
	FOdysseeItemView Output;
	TArray<FOdysseeItemView> Inputs;
};

DECLARE_DELEGATE_OneParam(FOdysseeCraftRequested, FName);
DECLARE_DELEGATE_OneParam(FOdysseeRecipeChosen, int32);

/** Une carte de la grille (image et nom de l'objet fabriqué), ou un onglet de catégorie. */
UCLASS()
class ODYSSE_API UOdysseeRecipeEntryWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Carte de recette (`Icon` vide : le nom seul) ou, si `bTab`, onglet de catégorie. */
	void Show(int32 InIndex, const FText& Text, const FString& Icon, bool bTab);
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
	TObjectPtr<UImage> Picture;
	UPROPERTY()
	TObjectPtr<UTextBlock> Label;
	int32 Index = 0;
	bool bIsTab = false;
};

UCLASS()
class ODYSSE_API UOdysseeCraftingWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Titre du panneau : « Artisanat » (à la main), « Établi », « Forge ». */
	void SetTitle(const FText& Title);

	/** Remplace les recettes (ordre de data/recipes.txt) ; les onglets suivent leurs catégories. */
	void ShowRecipes(const TArray<FOdysseeRecipeView>& Recipes);

	/** Ce que le joueur possède, par objet (inventaire) : « 3 / 8 » sous les ingrédients. */
	void ShowOwned(const TMap<FName, int32>& Owned);

	/** Fabrication commencée : la barre se remplit en `Seconds`. */
	void ShowCraftStarted(FName Recipe, float Seconds);
	/** Fin ou refus : message sous le bouton (`bSuccess` : en vert). */
	void ShowCraftMessage(const FText& Message, bool bSuccess);

	/** Le joueur demande à fabriquer la recette choisie : à transmettre au serveur. */
	FOdysseeCraftRequested OnCraftRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UWrapBox> CategoryTabs;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UUniformGridPanel> RecipeGrid;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UImage> OutputIcon;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> RecipeName;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UTextBlock> StationText;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UHorizontalBox> Formula;
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
	void BuildTabs();
	void FillGrid();
	void SelectTab(int32 Tab);
	void ChooseCard(int32 Card);
	void Select(int32 Recipe);
	void ShowFormula(const FOdysseeRecipeView& Recipe);
	UWidget* MakeItemTile(const FOdysseeItemView& Item, bool bIngredient, float Size);
	UFUNCTION()
	void HandleCraftClicked();

	TArray<FOdysseeRecipeView> Shown;
	TArray<FName> Tabs; ///< NAME_None : « Tout »
	TMap<FName, int32> OwnedItems;
	UPROPERTY()
	TArray<TObjectPtr<UOdysseeRecipeEntryWidget>> TabEntries;
	UPROPERTY()
	TArray<TObjectPtr<UOdysseeRecipeEntryWidget>> Cards;
	TArray<int32> CardRecipes; ///< recette de chaque carte affichée
	int32 SelectedTab = 0;
	int32 Selected = -1;
	float CraftElapsed = 0.f;
	float CraftDuration = 0.f; ///< 0 : pas de fabrication en cours
};
