#include "UI/OdysseeHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/OdysseeGaugeWidget.h"
#include "UI/OdysseeToolSlotWidget.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeHud"

namespace
{
	constexpr int32 ToolCount = 5;
	constexpr int32 MaxNotifications = 3;

	/** Place `Widget` dans le canevas, ancré à un coin ou au centre de l'écran. */
	void Pin(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Anchor, const FVector2D& Offset)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		CanvasSlot->SetAlignment(Anchor);
		CanvasSlot->SetPosition(Offset);
		CanvasSlot->SetAutoSize(true);
	}
}

UWidget* UOdysseeHudWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Pin(Canvas, BuildVitals(), FVector2D(0.f, 1.f), FVector2D(28.f, -28.f));
	Pin(Canvas, BuildToolBar(), FVector2D(0.5f, 1.f), FVector2D(0.f, -24.f));
	Pin(Canvas, BuildNetwork(), FVector2D(0.f, 0.f), FVector2D(20.f, 16.f));
	Pin(Canvas, BuildCrosshair(), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	NotificationBox = MakeColumn(TEXT("NotificationBox"));
	Pin(Canvas, NotificationBox, FVector2D(1.f, 0.f), FVector2D(-24.f, 24.f));
	return Canvas;
}

UWidget* UOdysseeHudWidget::BuildVitals()
{
	UVerticalBox* Bars = MakeColumn();
	UTextBlock* Title = MakeText(NAME_None, LOCTEXT("Vitals", "SURVIE"), 11, true, OdysseeUi::BoneDim);
	Title->SetFont(OdysseeUi::Font(11, true, 250));
	AddTo(Bars, Title, FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Left);
	AddTo(Bars, BuildVitalBar(TEXT("icone_sante"), OdysseeUi::Health, HealthBar, TEXT("HealthBar")));
	AddTo(Bars, BuildVitalBar(TEXT("icone_endurance"), OdysseeUi::Stamina, StaminaBar, TEXT("StaminaBar")));

	HungerGauge = WidgetTree->ConstructWidget<UOdysseeGaugeWidget>(UOdysseeGaugeWidget::StaticClass(),
		TEXT("HungerGauge"));
	HungerGauge->Configure(TEXT("icone_faim"), OdysseeUi::Hunger);
	ThirstGauge = WidgetTree->ConstructWidget<UOdysseeGaugeWidget>(UOdysseeGaugeWidget::StaticClass(),
		TEXT("ThirstGauge"));
	ThirstGauge->Configure(TEXT("icone_soif"), OdysseeUi::Thirst);

	UHorizontalBox* Row = MakeRow();
	AddTo(Row, Bars, FMargin(0.f, 0.f, 14.f, 0.f), VAlign_Bottom);
	AddTo(Row, HungerGauge.Get(), FMargin(4.f, 0.f), VAlign_Bottom);
	AddTo(Row, ThirstGauge.Get(), FMargin(4.f, 0.f), VAlign_Bottom);
	return MakePanel(NAME_None, Row, FMargin(18.f, 12.f, 16.f, 14.f));
}

UWidget* UOdysseeHudWidget::BuildVitalBar(const TCHAR* Icon, const FLinearColor& Color,
	TObjectPtr<UProgressBar>& OutBar, FName Name)
{
	OutBar = MakeBar(Name, Color, 1.f);
	UHorizontalBox* Row = MakeRow();
	AddTo(Row, MakeImage(NAME_None, Icon, FVector2D(18.f), Color), FMargin(0.f, 0.f, 8.f, 0.f));
	AddTo(Row, MakeSized(OutBar, FVector2D(230.f, 15.f)), FMargin(0.f));
	return Row;
}

UWidget* UOdysseeHudWidget::BuildToolBar()
{
	UHorizontalBox* Bar = MakeRow(TEXT("ToolBar"));
	for (int32 Index = 0; Index < ToolCount; ++Index)
	{
		UOdysseeToolSlotWidget* Tool = WidgetTree->ConstructWidget<UOdysseeToolSlotWidget>(
			UOdysseeToolSlotWidget::StaticClass(), *FString::Printf(TEXT("ToolSlot%d"), Index + 1));
		Tool->SetKey(Index + 1);
		ToolSlots.Add(Tool);
		AddTo(Bar, Tool, FMargin(5.f, 0.f));
	}
	ItemNameText = MakeText(TEXT("ItemNameText"), FText::GetEmpty(), 19, true);
	ItemNameText->SetFont(OdysseeUi::Font(19, true, 80));
	UVerticalBox* Column = MakeColumn();
	AddTo(Column, ItemNameText, FMargin(0.f, 0.f, 0.f, 8.f), HAlign_Center);
	AddTo(Column, Bar, FMargin(0.f), HAlign_Center);
	return Column;
}

UWidget* UOdysseeHudWidget::BuildNetwork()
{
	PingText = MakeText(TEXT("PingText"), FText::GetEmpty(), 11, true, OdysseeUi::BoneDim);
	ConnectionText = MakeText(TEXT("ConnectionText"), FText::GetEmpty(), 11, false, OdysseeUi::BoneDim);
	UHorizontalBox* Row = MakeRow();
	AddTo(Row, PingText, FMargin(0.f, 0.f, 12.f, 0.f));
	AddTo(Row, ConnectionText, FMargin(0.f));
	return Row;
}

UWidget* UOdysseeHudWidget::BuildCrosshair()
{
	// Une petite croix : discrète, comme sur les images de référence.
	UTextBlock* Mark = MakeText(NAME_None, FText::FromString(TEXT("+")), 16, false,
		FLinearColor(1.f, 1.f, 1.f, 0.75f));
	return Mark;
}

void UOdysseeHudWidget::AddNotification(const FText& Message)
{
	if (NotificationBox->GetChildrenCount() >= MaxNotifications)
	{
		NotificationBox->RemoveChildAt(0);
	}
	UTextBlock* Text = MakeText(NAME_None, Message, 13, false);
	UBorder* Panel = MakePanel(NAME_None, Text, FMargin(16.f, 8.f));
	AddTo(NotificationBox, Panel, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Right);
}

void UOdysseeHudWidget::ShowPreviewValues()
{
	HealthBar->SetPercent(0.7f);
	StaminaBar->SetPercent(0.45f);
	HungerGauge->SetProgress(0.6f);
	ThirstGauge->SetProgress(0.3f);
	const TCHAR* Icons[ToolCount] = {TEXT("outil_hache_pierre"), TEXT("outil_pioche_pierre"), TEXT("outil_lance"),
		TEXT("outil_lampe"), TEXT("outil_baton")};
	const int32 Counts[ToolCount] = {1, 1, 1, 1, 12};
	const float Wear[ToolCount] = {0.8f, 0.45f, 1.f, 0.15f, -1.f};
	for (int32 Index = 0; Index < ToolCount; ++Index)
	{
		ToolSlots[Index]->ShowItem(Icons[Index], Counts[Index], Wear[Index]);
		ToolSlots[Index]->SetActive(Index == 0);
	}
	ItemNameText->SetText(LOCTEXT("PreviewItem", "Hache de pierre"));
	PingText->SetText(LOCTEXT("PreviewPing", "42 ms"));
	ConnectionText->SetText(LOCTEXT("PreviewConnection", "Connecté · Monde persistant"));
	AddNotification(LOCTEXT("PreviewRegion", "Nouvelle région découverte : L'Abri"));
	AddNotification(LOCTEXT("PreviewLoot", "+3 Bûche"));
}

#undef LOCTEXT_NAMESPACE
