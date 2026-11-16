// Écran d'inventaire (touche Tab) : les 20 cases du sac, la barre d'outils (les 5 premières cases,
// les mêmes que le HUD) et la fiche de l'objet survolé. On déplace un objet en le glissant sur une
// autre case ; c'est le serveur qui décide (échange ou regroupement) et renvoie l'inventaire.
//
// Le jeu continue derrière (jamais de pause) : l'écran ne fait qu'afficher et transmettre.

#pragma once

#include "CoreMinimal.h"
#include "UI/OdysseeInventorySlotWidget.h"
#include "UI/OdysseeUiWidget.h"
#include "OdysseeInventoryWidget.generated.h"

class UImage;
class UUniformGridPanel;

/** Ce qu'affiche une case : icône (texture d'interface, vide si aucune), quantité, nom. */
struct FOdysseeSlotView
{
	FString Icon;
	int32 Count = 0;
	FText Name;

	bool operator==(const FOdysseeSlotView& Other) const
	{
		return Icon == Other.Icon && Count == Other.Count && Name.EqualTo(Other.Name);
	}
};

UCLASS()
class ODYSSE_API UOdysseeInventoryWidget : public UOdysseeUiWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotCount = 25;
	static constexpr int32 ToolbarCount = 5;

	/** Montre l'inventaire (25 cases, dans l'ordre du serveur) ; ne redessine que ce qui change. */
	void ShowSlots(const TArray<FOdysseeSlotView>& Views);

	/** L'objet d'une case a été lâché sur une autre : à transmettre au serveur. */
	FOdysseeSlotDropped OnMoveRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TArray<TObjectPtr<UOdysseeInventorySlotWidget>> Slots;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UImage> DetailIcon;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> DetailName;
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|UI")
	TObjectPtr<UTextBlock> DetailCount;

protected:
	virtual UWidget* BuildTree() override;

private:
	UWidget* BuildGrid();
	UWidget* BuildDetail();
	UOdysseeInventorySlotWidget* MakeSlot(int32 Index);
	void ShowDetail(int32 Index);
	void ForwardMove(int32 From, int32 To);

	TArray<FOdysseeSlotView> Shown;
	int32 Hovered = -1;
};
