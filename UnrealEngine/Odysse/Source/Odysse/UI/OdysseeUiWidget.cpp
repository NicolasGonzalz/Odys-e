#include "UI/OdysseeUiWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/OdysseeUiStyle.h"

bool UOdysseeUiWidget::Initialize()
{
	const bool bFirstTime = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = BuildTree();
	}
	return bFirstTime;
}

TSharedRef<SWidget> UOdysseeUiWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) // filet de sécurité (aperçu de l'éditeur UMG)
	{
		WidgetTree->RootWidget = BuildTree();
	}
	return Super::RebuildWidget();
}

UTextBlock* UOdysseeUiWidget::MakeText(FName Name, const FText& Text, int32 Size, bool bBold,
	const FLinearColor& Color)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Block->SetText(Text);
	Block->SetFont(OdysseeUi::Font(Size, bBold));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(1.f, 1.f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	return Block;
}

UButton* UOdysseeUiWidget::MakeButton(FName Name, const FText& Label, float Width)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetStyle(OdysseeUi::ButtonStyle());
	UTextBlock* Text = MakeText(NAME_None, Label.ToUpper(), 15, true);
	Text->SetFont(OdysseeUi::Font(15, true, 120));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(Width);
	Box->SetHeightOverride(52.f);
	Box->AddChild(Button);
	return Button;
}

void UOdysseeUiWidget::AddButton(UVerticalBox* Column, UButton* Button)
{
	AddTo(Column, Button->GetParent(), FMargin(0.f, 5.f), HAlign_Center);
}

UBorder* UOdysseeUiWidget::MakePanel(FName Name, UWidget* Content, const FMargin& Inset)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Panel->SetBrush(OdysseeUi::BoxBrush(TEXT("panneau"), FMargin(0.18f)));
	Panel->SetPadding(Inset);
	Panel->SetContent(Content);
	return Panel;
}

UImage* UOdysseeUiWidget::MakeImage(FName Name, const TCHAR* Texture, const FVector2D& Size,
	const FLinearColor& Tint)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
	Image->SetBrush(OdysseeUi::ImageBrush(Texture, Size));
	Image->SetColorAndOpacity(Tint);
	return Image;
}

UProgressBar* UOdysseeUiWidget::MakeBar(FName Name, const FLinearColor& Fill, float Percent)
{
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
	Bar->SetWidgetStyle(OdysseeUi::BarStyle());
	Bar->SetFillColorAndOpacity(Fill);
	Bar->SetPercent(Percent);
	return Bar;
}

USizeBox* UOdysseeUiWidget::MakeSized(UWidget* Content, const FVector2D& Size)
{
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(Size.X);
	Box->SetHeightOverride(Size.Y);
	Box->AddChild(Content);
	return Box;
}

UVerticalBox* UOdysseeUiWidget::MakeColumn(FName Name)
{
	return WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
}

UHorizontalBox* UOdysseeUiWidget::MakeRow(FName Name)
{
	return WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
}

void UOdysseeUiWidget::AddTo(UVerticalBox* Column, UWidget* Child, const FMargin& Inset,
	EHorizontalAlignment Align)
{
	UVerticalBoxSlot* BoxSlot = Column->AddChildToVerticalBox(Child);
	BoxSlot->SetPadding(Inset);
	BoxSlot->SetHorizontalAlignment(Align);
}

void UOdysseeUiWidget::AddTo(UHorizontalBox* Row, UWidget* Child, const FMargin& Inset,
	EVerticalAlignment Align)
{
	UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(Child);
	RowSlot->SetPadding(Inset);
	RowSlot->SetVerticalAlignment(Align);
}

UBorder* UOdysseeUiWidget::MakeMenuPanel(FName Name, const FText& Title, UVerticalBox*& OutColumn)
{
	UVerticalBox* Column = MakeColumn();
	UTextBlock* Heading = MakeText(NAME_None, Title.ToUpper(), 22, true, OdysseeUi::Rust);
	Heading->SetFont(OdysseeUi::Font(22, true, 300));
	AddTo(Column, Heading, FMargin(0.f, 0.f, 0.f, 14.f), HAlign_Center);
	OutColumn = MakeColumn();
	AddTo(Column, OutColumn, FMargin(0.f), HAlign_Center);
	return MakePanel(Name, Column, FMargin(36.f, 28.f));
}
