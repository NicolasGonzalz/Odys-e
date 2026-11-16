#include "UI/OdysseeSurvivorChoiceWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "OdysseeSurvivorChoice"

namespace
{
	const FVector2D CardSize(300.f, 360.f);

	void Fill(UOverlaySlot* Cell, EHorizontalAlignment H = HAlign_Fill, EVerticalAlignment V = VAlign_Fill,
		const FMargin& Space = FMargin(0.f))
	{
		Cell->SetHorizontalAlignment(H);
		Cell->SetVerticalAlignment(V);
		Cell->SetPadding(Space);
	}
}

// --- Une carte -----------------------------------------------------------------------------------

UWidget* UOdysseeSurvivorCardWidget::BuildTree()
{
	CapsuleText = MakeText(TEXT("CapsuleText"), FText::GetEmpty(), 12, true, DimColor());
	CapsuleText->SetFont(OdysseeUi::Font(12, true, 250));
	NameText = MakeText(TEXT("NameText"), FText::GetEmpty(), 26, true);
	SpecialtyText = MakeText(TEXT("SpecialtyText"), FText::GetEmpty(), 15, true, AccentColor());
	SpecialtyText->SetFont(OdysseeUi::Font(15, true, 150));
	SkillText = MakeText(TEXT("SkillText"), FText::GetEmpty(), 14, false, DimColor());
	SkillText->SetAutoWrapText(true);
	SkillText->SetJustification(ETextJustify::Center);
	ChooseButton = MakeButton(TEXT("ChooseButton"), LOCTEXT("Choose", "Incarner"), 220.f);
	ChooseLabel = Cast<UTextBlock>(ChooseButton->GetChildAt(0));
	ChooseButton->OnClicked.AddDynamic(this, &UOdysseeSurvivorCardWidget::HandleClicked);

	UVerticalBox* Column = MakeColumn();
	AddTo(Column, CapsuleText, FMargin(0.f, 0.f, 0.f, 18.f), HAlign_Center);
	AddTo(Column, NameText, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Center);
	AddTo(Column, SpecialtyText, FMargin(0.f, 0.f, 0.f, 16.f), HAlign_Center);
	AddTo(Column, MakeSized(SkillText, FVector2D(CardSize.X - 56.f, 120.f)), FMargin(0.f), HAlign_Center);
	AddButton(Column, ChooseButton);
	return MakePanel(TEXT("Card"), MakeSized(Column, CardSize), FMargin(28.f, 26.f));
}

void UOdysseeSurvivorCardWidget::Show(const FOdysseeSurvivorView& View)
{
	Index = View.Index;
	CapsuleText->SetText(FText::Format(LOCTEXT("Capsule", "CAPSULE {0}"), View.Capsule));
	NameText->SetText(View.Name);
	SpecialtyText->SetText(View.Specialty.ToUpper());
	SkillText->SetText(View.Skill);
	ChooseButton->SetIsEnabled(!View.bTaken);
	ChooseLabel->SetText(View.bTaken ? LOCTEXT("Taken", "DÉJÀ INCARNÉ") : LOCTEXT("ChooseUpper", "INCARNER"));
	SetRenderOpacity(View.bTaken ? 0.55f : 1.f); // pris par un autre joueur : grisé
}

void UOdysseeSurvivorCardWidget::HandleClicked()
{
	OnChosen.ExecuteIfBound(Index);
}

// --- L'écran ------------------------------------------------------------------------------------

UWidget* UOdysseeSurvivorChoiceWidget::BuildTree()
{
	UTextBlock* Title = MakeText(NAME_None, LOCTEXT("Title", "QUI ÊTES-VOUS ?"), 40, true);
	Title->SetFont(OdysseeUi::Font(40, true, 350));
	UTextBlock* Subtitle = MakeText(NAME_None, LOCTEXT("Subtitle",
		"Choisissez le survivant que vous incarnez dans ce monde. Chacun ne peut être incarné que par un "
		"seul joueur."), 15, false, DimColor());
	Cards = MakeRow(TEXT("Cards"));
	StatusText = MakeText(TEXT("StatusText"), FText::GetEmpty(), 15, true, AccentColor());
	BackButton = MakeButton(TEXT("BackButton"), LOCTEXT("Back", "Retour"), 220.f);

	UVerticalBox* Column = MakeColumn();
	AddTo(Column, Title, FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Center);
	AddTo(Column, Subtitle, FMargin(0.f, 0.f, 0.f, 30.f), HAlign_Center);
	AddTo(Column, Cards, FMargin(0.f), HAlign_Center);
	AddTo(Column, StatusText, FMargin(0.f, 18.f, 0.f, 10.f), HAlign_Center);
	AddButton(Column, BackButton);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Fill(Root->AddChildToOverlay(BuildBackground()));
	Fill(Root->AddChildToOverlay(Column), HAlign_Center, VAlign_Center, FMargin(40.f));
	return Root;
}

UWidget* UOdysseeSurvivorChoiceWidget::BuildBackground()
{
	// Le même fond que le menu principal, plus voilé : le choix est au premier plan.
	UScaleBox* Cover = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	Cover->SetStretch(EStretch::ScaleToFill);
	Cover->AddChild(MakeImage(NAME_None, TEXT("menu_fond"), FVector2D(1920.f, 900.f)));
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Veil->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.02f, 0.65f));
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Fill(Layers->AddChildToOverlay(Cover));
	Fill(Layers->AddChildToOverlay(Veil));
	return Layers;
}

void UOdysseeSurvivorChoiceWidget::ShowSurvivors(const TArray<FOdysseeSurvivorView>& Survivors)
{
	Cards->ClearChildren();
	for (const FOdysseeSurvivorView& View : Survivors)
	{
		UOdysseeSurvivorCardWidget* Card = MakeChild<UOdysseeSurvivorCardWidget>();
		Card->Show(View);
		Card->OnChosen.BindUObject(this, &UOdysseeSurvivorChoiceWidget::ForwardChoice);
		AddTo(Cards, Card, FMargin(10.f, 0.f), VAlign_Top);
	}
}

void UOdysseeSurvivorChoiceWidget::ShowStatus(const FText& Status)
{
	StatusText->SetText(Status);
}

void UOdysseeSurvivorChoiceWidget::ForwardChoice(int32 Index)
{
	OnSurvivorChosen.ExecuteIfBound(Index);
}

#undef LOCTEXT_NAMESPACE
