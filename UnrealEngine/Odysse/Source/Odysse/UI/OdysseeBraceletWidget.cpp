#include "UI/OdysseeBraceletWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/OdysseeCraftingWidget.h"
#include "UI/OdysseeInventoryWidget.h"

#define LOCTEXT_NAMESPACE "OdysseeBracelet"

namespace
{
	constexpr float IgniteSeconds = 0.3f;
	constexpr float SweepSeconds = 3.5f; ///< une bande de balayage descend sur l'hologramme
}

UWidget* UOdysseeBraceletWidget::BuildTree()
{
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	Inventory = MakeChild<UOdysseeInventoryWidget>(TEXT("Bag"));
	Crafting = MakeChild<UOdysseeCraftingWidget>(TEXT("Crafting"));
	Pages->AddChild(Inventory); // même ordre que EOdysseeBraceletTab
	Pages->AddChild(Crafting);
	Pages->AddChild(BuildComingSoon(LOCTEXT("SurvivalTitle", "Carnet de survie"),
		LOCTEXT("SurvivalText", "Ce que vous apprenez en récoltant, en chassant, en fabriquant et en mangeant "
			"s'inscrira ici, avec vos besoins vitaux en détail.")));
	Pages->AddChild(BuildComingSoon(LOCTEXT("StoryTitle", "Histoire"),
		LOCTEXT("StoryText", "La frise chronologique, les indices lus et vos souvenirs.")));
	Pages->AddChild(BuildComingSoon(LOCTEXT("MapTitle", "Carte"),
		LOCTEXT("MapText", "Les régions connues, les souvenirs à décharger à la table de cartographie, "
			"les joueurs et la boussole.")));

	UVerticalBox* Column = MakeColumn();
	UTextBlock* Header = MakeText(NAME_None, LOCTEXT("Header", "BRACELET DE STASE · ODYSSÉE"), 11, true, DimColor());
	Header->SetFont(OdysseeUi::Font(11, true, 300));
	AddTo(Column, Header, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Center);
	AddTo(Column, BuildTabs(), FMargin(0.f, 0.f, 0.f, 10.f), HAlign_Center);
	AddTo(Column, Pages, FMargin(0.f), HAlign_Center);

	// La bande de balayage passe par-dessus le contenu, sans capter la souris.
	Sweep = MakeImage(TEXT("Sweep"), TEXT("holo_balayage"), FVector2D(256.f, 64.f));
	Sweep->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlay* Projection = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Projection->AddChildToOverlay(Column);
	// Lignes de balayage fines, répétées (et non étirées) sur toute la projection.
	UImage* Lines = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ScanLines"));
	FSlateBrush LineBrush = OdysseeUi::ImageBrush(TEXT("holo_lignes"), FVector2D(8.f, 3.f), Theme);
	LineBrush.Tiling = ESlateBrushTileType::Both;
	Lines->SetBrush(LineBrush);
	Lines->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* LinesSlot = Projection->AddChildToOverlay(Lines);
	LinesSlot->SetHorizontalAlignment(HAlign_Fill);
	LinesSlot->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* SweepSlot = Projection->AddChildToOverlay(Sweep);
	SweepSlot->SetHorizontalAlignment(HAlign_Fill);
	SweepSlot->SetVerticalAlignment(VAlign_Top);
	Content = Column;

	// Assombrissement bleu nuit : l'hologramme se lit sur un ciel clair ; le jeu reste visible.
	UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Shade->SetBrushColor(FLinearColor(0.f, 0.015f, 0.035f, 0.45f));
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Root->AddChildToOverlay(Shade);
	Back->SetHorizontalAlignment(HAlign_Fill);
	Back->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* Front = Root->AddChildToOverlay(Projection);
	Front->SetHorizontalAlignment(HAlign_Center);
	Front->SetVerticalAlignment(VAlign_Center);
	return Root;
}

UWidget* UOdysseeBraceletWidget::BuildTabs()
{
	const FText Labels[] = {LOCTEXT("Bag", "Sac"), LOCTEXT("Crafting", "Fabrication"), LOCTEXT("Survival", "Survie"),
		LOCTEXT("Story", "Histoire"), LOCTEXT("Map", "Carte")};
	UHorizontalBox* Row = MakeRow(TEXT("Tabs"));
	for (int32 Tab = 0; Tab < UE_ARRAY_COUNT(Labels); ++Tab)
	{
		UOdysseeRecipeEntryWidget* Entry = MakeChild<UOdysseeRecipeEntryWidget>();
		Entry->Show(Tab, Labels[Tab], FString(), true);
		Entry->OnChosen.BindUObject(this, &UOdysseeBraceletWidget::ChooseTab);
		AddTo(Row, Entry, FMargin(4.f, 0.f));
		Tabs.Add(Entry);
	}
	return Row;
}

UWidget* UOdysseeBraceletWidget::BuildComingSoon(const FText& Title, const FText& Text)
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, Title, Column);
	UTextBlock* Body = MakeText(NAME_None, Text, 15);
	Body->SetAutoWrapText(true);
	Body->SetJustification(ETextJustify::Center);
	AddTo(Column, MakeSized(Body, FVector2D(560.f, 120.f)), FMargin(0.f, 10.f), HAlign_Center);
	AddTo(Column, MakeText(NAME_None, LOCTEXT("Soon", "BIENTÔT"), 12, true, DimColor()), FMargin(0.f), HAlign_Center);
	return Panel;
}

void UOdysseeBraceletWidget::ChooseTab(int32 Tab)
{
	ShowTab(static_cast<EOdysseeBraceletTab>(Tab));
}

void UOdysseeBraceletWidget::ShowTab(EOdysseeBraceletTab Tab)
{
	Current = Tab;
	const int32 Index = static_cast<int32>(Tab);
	Pages->SetActiveWidgetIndex(Index);
	for (int32 Entry = 0; Entry < Tabs.Num(); ++Entry)
	{
		Tabs[Entry]->SetSelected(Entry == Index);
	}
}

void UOdysseeBraceletWidget::PlayOpen()
{
	OpenTime = 0.f;
	SetRenderOpacity(0.f);
}

void UOdysseeBraceletWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	// Balayage : du haut au bas du contenu, puis on recommence.
	SweepTime = FMath::Fmod(SweepTime + DeltaTime, SweepSeconds);
	const float Height = Content ? Content->GetDesiredSize().Y : 0.f;
	Sweep->SetRenderTranslation(FVector2D(0.f, (SweepTime / SweepSeconds) * Height - 32.f));
	if (OpenTime >= IgniteSeconds)
	{
		return;
	}
	// Allumage : l'hologramme apparaît en scintillant, comme une projection qui se stabilise.
	OpenTime += DeltaTime;
	const float Ramp = FMath::Clamp(OpenTime / IgniteSeconds, 0.f, 1.f);
	const float Flicker = 0.8f + 0.2f * FMath::Sin(OpenTime * 90.f);
	SetRenderOpacity(Ramp >= 1.f ? 1.f : Ramp * Flicker);
}

#undef LOCTEXT_NAMESPACE
