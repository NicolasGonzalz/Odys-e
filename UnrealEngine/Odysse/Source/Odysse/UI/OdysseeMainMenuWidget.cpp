#include "UI/OdysseeMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/OdysseeOptionsWidgets.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeMainMenu"

namespace
{
	// Ordre des pages dans le WidgetSwitcher.
	enum class EPage : int32 { Root, Solo, Multiplayer, Options, KeyBindings, Settings };

	void Fill(UOverlaySlot* Cell, EHorizontalAlignment H = HAlign_Fill, EVerticalAlignment V = VAlign_Fill,
		const FMargin& Padding = FMargin(0.f))
	{
		Cell->SetHorizontalAlignment(H);
		Cell->SetVerticalAlignment(V);
		Cell->SetPadding(Padding);
	}
}

UWidget* UOdysseeMainMenuWidget::BuildTree()
{
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	Pages->AddChild(BuildRootPage());
	Pages->AddChild(BuildSoloPage());
	Pages->AddChild(BuildMultiplayerPage());
	Pages->AddChild(BuildOptionsPage());
	KeyBindings = WidgetTree->ConstructWidget<UOdysseeKeyBindingsWidget>(
		UOdysseeKeyBindingsWidget::StaticClass(), TEXT("KeyBindings"));
	KeyBindings->BackButton->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowOptions);
	Pages->AddChild(KeyBindings);
	Settings = WidgetTree->ConstructWidget<UOdysseeSettingsWidget>(UOdysseeSettingsWidget::StaticClass(),
		TEXT("Settings"));
	Settings->BackButton->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowOptions);
	Settings->KeyBindingsButton->GetParent()->SetVisibility(ESlateVisibility::Collapsed); // déjà dans Options
	Pages->AddChild(Settings);

	UVerticalBox* Left = MakeColumn();
	AddTo(Left, BuildTitle(), FMargin(0.f, 0.f, 0.f, 36.f), HAlign_Left);
	AddTo(Left, Pages, FMargin(0.f), HAlign_Left);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Fill(Root->AddChildToOverlay(BuildBackground()));
	Fill(Root->AddChildToOverlay(Left), HAlign_Left, VAlign_Center, FMargin(96.f, 40.f));
	UTextBlock* Version = MakeText(NAME_None, LOCTEXT("Version", "Version de développement"), 11, false,
		OdysseeUi::BoneDim);
	Fill(Root->AddChildToOverlay(Version), HAlign_Right, VAlign_Bottom, FMargin(24.f, 18.f));
	return Root;
}

UWidget* UOdysseeMainMenuWidget::BuildBackground()
{
	// L'image remplit l'écran sans être déformée ; un voile sombre garde le texte lisible.
	UScaleBox* Cover = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	Cover->SetStretch(EStretch::ScaleToFill);
	Cover->AddChild(MakeImage(TEXT("Background"), TEXT("menu_fond"), FVector2D(1920.f, 900.f)));
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Veil->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.02f, 0.45f));
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Fill(Layers->AddChildToOverlay(Cover));
	Fill(Layers->AddChildToOverlay(Veil));
	return Layers;
}

UWidget* UOdysseeMainMenuWidget::BuildTitle()
{
	UVerticalBox* Column = MakeColumn();
	UTextBlock* Title = MakeText(TEXT("TitleText"), LOCTEXT("Title", "ODYSSÉE"), 72, true);
	Title->SetFont(OdysseeUi::Font(72, true, 450));
	Title->SetShadowOffset(FVector2D(3.f, 3.f));
	AddTo(Column, Title, FMargin(0.f), HAlign_Left);
	UTextBlock* Tagline = MakeText(NAME_None, LOCTEXT("Tagline", "Survivre. Explorer. Comprendre."), 18, false,
		OdysseeUi::Rust);
	Tagline->SetFont(OdysseeUi::Font(18, false, 120));
	AddTo(Column, Tagline, FMargin(6.f, 0.f, 0.f, 0.f), HAlign_Left);
	return Column;
}

UWidget* UOdysseeMainMenuWidget::BuildRootPage()
{
	UVerticalBox* Column = MakeColumn();
	SoloButton = MakeButton(TEXT("SoloButton"), LOCTEXT("Solo", "Solo"));
	MultiplayerButton = MakeButton(TEXT("MultiplayerButton"), LOCTEXT("Multi", "Multijoueur"));
	OptionsButton = MakeButton(TEXT("OptionsButton"), LOCTEXT("Options", "Options"));
	QuitButton = MakeButton(TEXT("QuitButton"), LOCTEXT("Quit", "Quitter"));
	for (UButton* Button : {SoloButton.Get(), MultiplayerButton.Get(), OptionsButton.Get(), QuitButton.Get()})
	{
		AddButton(Column, Button);
	}
	SoloButton->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowSolo);
	MultiplayerButton->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowMultiplayer);
	OptionsButton->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowOptions);
	return Column;
}

UWidget* UOdysseeMainMenuWidget::BuildSoloPage()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("SoloTitle", "Solo"), Column);
	WorldList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("WorldList"));
	// Mondes de démonstration : la vraie liste viendra des sauvegardes (T053).
	WorldList->AddChild(MakeButton(NAME_None, LOCTEXT("World1", "Ma première partie · 2 h 14"), 400.f)->GetParent());
	WorldList->AddChild(MakeButton(NAME_None, LOCTEXT("World2", "Monde d'essai · 25 min"), 400.f)->GetParent());
	AddTo(Column, MakeSized(WorldList, FVector2D(410.f, 140.f)), FMargin(0.f, 0.f, 0.f, 12.f));
	NewWorldButton = MakeButton(TEXT("NewWorldButton"), LOCTEXT("NewWorld", "Nouvelle partie"));
	LoadWorldButton = MakeButton(TEXT("LoadWorldButton"), LOCTEXT("LoadWorld", "Charger"));
	AddButton(Column, NewWorldButton);
	AddButton(Column, LoadWorldButton);
	AddButton(Column, MakeBackButton(GET_FUNCTION_NAME_CHECKED(UOdysseeMainMenuWidget, ShowRoot)));
	return Panel;
}

UWidget* UOdysseeMainMenuWidget::BuildMultiplayerPage()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("MultiTitle", "Multijoueur"), Column);
	AddTo(Column, MakeText(NAME_None, LOCTEXT("JoinLabel", "Rejoindre avec un code"), 14, true), FMargin(0.f, 0.f,
		0.f, 6.f), HAlign_Left);
	AddressInput = MakeField(TEXT("AddressInput"), LOCTEXT("AddressHint", "Adresse (89.92.152.61:58121)"));
	NameInput = MakeField(TEXT("NameInput"), LOCTEXT("NameHint", "Pseudo"));
	AddTo(Column, MakeSized(AddressInput, FVector2D(410.f, 48.f)), FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Left);
	AddTo(Column, MakeSized(NameInput, FVector2D(410.f, 48.f)), FMargin(0.f, 0.f, 0.f, 6.f), HAlign_Left);
	AddTo(Column, BuildJoinRow(), FMargin(0.f, 0.f, 0.f, 4.f));
	JoinStatusText = MakeText(TEXT("JoinStatusText"), FText::GetEmpty(), 12, false, OdysseeUi::BoneDim);
	AddTo(Column, JoinStatusText, FMargin(0.f, 0.f, 0.f, 12.f), HAlign_Left);
	AddTo(Column, MakeText(NAME_None, LOCTEXT("Favorites", "Serveurs favoris"), 14, true), FMargin(0.f, 0.f, 0.f,
		6.f), HAlign_Left);
	FavoritesList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("FavoritesList"));
	FavoritesList->AddChild(
		MakeButton(NAME_None, LOCTEXT("Homelab", "Monde persistant (homelab)"), 400.f)->GetParent());
	AddTo(Column, MakeSized(FavoritesList, FVector2D(410.f, 70.f)), FMargin(0.f, 0.f, 0.f, 12.f));
	HostButton = MakeButton(TEXT("HostButton"), LOCTEXT("Host", "Héberger ma partie"));
	AddButton(Column, HostButton);
	AddButton(Column, MakeBackButton(GET_FUNCTION_NAME_CHECKED(UOdysseeMainMenuWidget, ShowRoot)));
	return Panel;
}

UEditableTextBox* UOdysseeMainMenuWidget::MakeField(FName Name, const FText& Hint)
{
	UEditableTextBox* Input = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), Name);
	const FSlateBrush Field = OdysseeUi::BoxBrush(TEXT("champ"), FMargin(12.f / 256.f, 12.f / 48.f));
	Input->WidgetStyle.SetBackgroundImageNormal(Field).SetBackgroundImageHovered(Field)
		.SetBackgroundImageFocused(Field).SetFont(OdysseeUi::Font(16, true, 200))
		.SetForegroundColor(FSlateColor(OdysseeUi::Bone)).SetPadding(FMargin(14.f, 10.f));
	Input->SetHintText(Hint);
	return Input;
}

UWidget* UOdysseeMainMenuWidget::BuildJoinRow()
{
	JoinCodeInput = MakeField(TEXT("JoinCodeInput"), LOCTEXT("CodeHint", "ODY-XXXX"));
	JoinButton = MakeButton(TEXT("JoinButton"), LOCTEXT("Join", "Rejoindre"), 150.f);
	UHorizontalBox* Row = MakeRow();
	AddTo(Row, MakeSized(JoinCodeInput, FVector2D(250.f, 48.f)), FMargin(0.f, 0.f, 10.f, 0.f));
	AddTo(Row, JoinButton->GetParent(), FMargin(0.f));
	return Row;
}

UWidget* UOdysseeMainMenuWidget::BuildOptionsPage()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("OptionsTitle", "Options"), Column);
	UButton* Keys = MakeButton(TEXT("KeyBindingsButton"), LOCTEXT("Keys", "Touches"));
	UButton* Graphics = MakeButton(TEXT("SettingsButton"), LOCTEXT("Settings", "Paramètres"));
	Keys->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowKeyBindings);
	Graphics->OnClicked.AddDynamic(this, &UOdysseeMainMenuWidget::ShowSettings);
	AddButton(Column, Keys);
	AddButton(Column, Graphics);
	AddButton(Column, MakeBackButton(GET_FUNCTION_NAME_CHECKED(UOdysseeMainMenuWidget, ShowRoot)));
	return Panel;
}

UButton* UOdysseeMainMenuWidget::MakeBackButton(FName Target)
{
	UButton* Back = MakeButton(NAME_None, LOCTEXT("Back", "Retour"));
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, Target);
	Back->OnClicked.Add(Delegate);
	return Back;
}

void UOdysseeMainMenuWidget::ShowRoot() { Pages->SetActiveWidgetIndex(int32(EPage::Root)); }
void UOdysseeMainMenuWidget::ShowSolo() { Pages->SetActiveWidgetIndex(int32(EPage::Solo)); }
void UOdysseeMainMenuWidget::ShowMultiplayer() { Pages->SetActiveWidgetIndex(int32(EPage::Multiplayer)); }
void UOdysseeMainMenuWidget::ShowOptions() { Pages->SetActiveWidgetIndex(int32(EPage::Options)); }
void UOdysseeMainMenuWidget::ShowKeyBindings() { Pages->SetActiveWidgetIndex(int32(EPage::KeyBindings)); }
void UOdysseeMainMenuWidget::ShowSettings() { Pages->SetActiveWidgetIndex(int32(EPage::Settings)); }

#undef LOCTEXT_NAMESPACE
