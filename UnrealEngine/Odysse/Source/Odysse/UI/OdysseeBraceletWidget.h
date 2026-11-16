// Le bracelet de stase (docs/BRACELET.md) : un hologramme à cinq onglets, Sac, Fabrication, Survie,
// Histoire et Carte. Il remplace les écrans séparés ; le temps continue pendant qu'on le consulte.
//
// Pour l'instant affiché à plat devant le joueur ; le geste « lever le poignet » et la projection
// au-dessus du bracelet (T079) le placeront dans le monde sans le refaire.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeBraceletWidget.generated.h"

class UOdysseeCraftingWidget;
class UOdysseeInventoryWidget;
class UOdysseeRecipeEntryWidget;
class UImage;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class EOdysseeBraceletTab : uint8
{
	Bag,
	Crafting,
	Survival,
	Story,
	Map,
};

UCLASS()
class ODYSSE_API UOdysseeBraceletWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	/** Montre un onglet (et rejoue l'allumage de l'hologramme si le bracelet vient de s'ouvrir). */
	void ShowTab(EOdysseeBraceletTab Tab);
	EOdysseeBraceletTab GetTab() const { return Current; }

	/** À appeler quand le bracelet s'ouvre : l'hologramme s'allume. */
	void PlayOpen();

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeInventoryWidget> Inventory;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UOdysseeCraftingWidget> Crafting;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI") TObjectPtr<UWidgetSwitcher> Pages;

protected:
	virtual EOdysseeUiTheme ChooseTheme() const override { return EOdysseeUiTheme::Holo; }
	virtual UWidget* BuildTree() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

private:
	UWidget* BuildTabs();
	UWidget* BuildComingSoon(const FText& Title, const FText& Text);
	void ChooseTab(int32 Tab);

	UPROPERTY()
	TArray<TObjectPtr<UOdysseeRecipeEntryWidget>> Tabs;
	UPROPERTY()
	TObjectPtr<UWidget> Content;
	UPROPERTY()
	TObjectPtr<UImage> Sweep;
	float SweepTime = 0.f;
	EOdysseeBraceletTab Current = EOdysseeBraceletTab::Bag;
	float OpenTime = 1.f; ///< secondes depuis l'ouverture (l'allumage dure 0,3 s)
};
