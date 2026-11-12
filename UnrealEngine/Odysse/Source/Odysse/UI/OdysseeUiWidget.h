// Base des écrans d'interface d'Odyssée : chaque écran construit son arbre de widgets en C++
// (BuildTree), avec les briques communes ci-dessous et la direction visuelle d'OdysseeUiStyle.
//
// Aucune logique de jeu ici : les écrans affichent ce qu'on leur donne. Les éléments que le jeu
// pilotera ont un nom fixe (HealthBar, ToolSlot1…) et un pointeur public en lecture.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/SizeBox.h"
#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"
#include "OdysseeUiWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

UCLASS(Abstract)
class ODYSSE_API UOdysseeUiWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Construit l'arbre dès la création : un écran peut régler ses sous-widgets aussitôt. */
	virtual bool Initialize() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Racine de l'écran, construite une seule fois. */
	virtual UWidget* BuildTree() PURE_VIRTUAL(UOdysseeUiWidget::BuildTree, return nullptr;);

	UTextBlock* MakeText(FName Name, const FText& Text, int32 Size, bool bBold = false,
		const FLinearColor& Color = FLinearColor(0.93f, 0.89f, 0.80f));
	/** Bouton low-poly avec un libellé en majuscules, dans une boîte à sa taille : on l'ajoute à
	 *  une colonne avec AddButton (qui place la boîte, pas le bouton seul). */
	UButton* MakeButton(FName Name, const FText& Label, float Width = 320.f);
	static void AddButton(UVerticalBox* Column, UButton* Button);
	/** Panneau facetté aux coins coupés autour de `Content`. */
	UBorder* MakePanel(FName Name, UWidget* Content, const FMargin& Inset = FMargin(24.f));
	UImage* MakeImage(FName Name, const TCHAR* Texture, const FVector2D& Size,
		const FLinearColor& Tint = FLinearColor::White);
	UProgressBar* MakeBar(FName Name, const FLinearColor& Fill, float Percent);
	USizeBox* MakeSized(UWidget* Content, const FVector2D& Size);

	UVerticalBox* MakeColumn(FName Name = NAME_None);
	UHorizontalBox* MakeRow(FName Name = NAME_None);
	static void AddTo(UVerticalBox* Column, UWidget* Child, const FMargin& Inset = FMargin(0.f, 4.f),
		EHorizontalAlignment Align = HAlign_Fill);
	static void AddTo(UHorizontalBox* Row, UWidget* Child, const FMargin& Inset = FMargin(4.f, 0.f),
		EVerticalAlignment Align = VAlign_Center);

	/** Titre d'écran, puis une colonne de boutons, dans un panneau. */
	UBorder* MakeMenuPanel(FName Name, const FText& Title, UVerticalBox*& OutColumn);
};
