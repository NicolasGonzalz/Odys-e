#include "UI/OdysseePauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/OdysseeOptionsWidgets.h"

#define LOCTEXT_NAMESPACE "OdysseePauseMenu"

namespace
{
	enum class EPausePage : int32 { Root, KeyBindings, Settings };
}

UWidget* UOdysseePauseMenuWidget::BuildTree()
{
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	Pages->AddChild(BuildRootPage());
	KeyBindings = WidgetTree->ConstructWidget<UOdysseeKeyBindingsWidget>(
		UOdysseeKeyBindingsWidget::StaticClass(), TEXT("KeyBindings"));
	KeyBindings->BackButton->OnClicked.AddDynamic(this, &UOdysseePauseMenuWidget::ShowRoot);
	Pages->AddChild(KeyBindings);
	Settings = WidgetTree->ConstructWidget<UOdysseeSettingsWidget>(UOdysseeSettingsWidget::StaticClass(),
		TEXT("Settings"));
	Settings->BackButton->OnClicked.AddDynamic(this, &UOdysseePauseMenuWidget::ShowRoot);
	Pages->AddChild(Settings);

	// Le jeu reste visible derrière un voile : en coop, le monde continue pendant la pause.
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* Back = Root->AddChildToOverlay(Veil);
	Back->SetHorizontalAlignment(HAlign_Fill);
	Back->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* Front = Root->AddChildToOverlay(Pages);
	Front->SetHorizontalAlignment(HAlign_Center);
	Front->SetVerticalAlignment(VAlign_Center);
	return Root;
}

UWidget* UOdysseePauseMenuWidget::BuildRootPage()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("Title", "Pause"), Column);
	ResumeButton = MakeButton(TEXT("ResumeButton"), LOCTEXT("Resume", "Reprendre"));
	KeyBindingsButton = MakeButton(TEXT("KeyBindingsButton"), LOCTEXT("Keys", "Modifier les touches"));
	SettingsButton = MakeButton(TEXT("SettingsButton"), LOCTEXT("Settings", "Paramètres"));
	SaveButton = MakeButton(TEXT("SaveButton"), LOCTEXT("Save", "Enregistrer"));
	LoadButton = MakeButton(TEXT("LoadButton"), LOCTEXT("Load", "Charger une partie"));
	NewGameButton = MakeButton(TEXT("NewGameButton"), LOCTEXT("NewGame", "Nouvelle partie"));
	QuitButton = MakeButton(TEXT("QuitButton"), LOCTEXT("Quit", "Quitter"));
	for (UButton* Button : {ResumeButton.Get(), KeyBindingsButton.Get(), SettingsButton.Get(), SaveButton.Get(),
			 LoadButton.Get(), NewGameButton.Get(), QuitButton.Get()})
	{
		AddButton(Column, Button);
	}
	KeyBindingsButton->OnClicked.AddDynamic(this, &UOdysseePauseMenuWidget::ShowKeyBindings);
	SettingsButton->OnClicked.AddDynamic(this, &UOdysseePauseMenuWidget::ShowSettings);
	return Panel;
}

void UOdysseePauseMenuWidget::SetCoopMode(bool bCoop)
{
	SaveButton->SetIsEnabled(!bCoop);
	LoadButton->SetIsEnabled(!bCoop);
	NewGameButton->SetIsEnabled(!bCoop);
}

void UOdysseePauseMenuWidget::ShowRoot() { Pages->SetActiveWidgetIndex(int32(EPausePage::Root)); }
void UOdysseePauseMenuWidget::ShowKeyBindings() { Pages->SetActiveWidgetIndex(int32(EPausePage::KeyBindings)); }
void UOdysseePauseMenuWidget::ShowSettings() { Pages->SetActiveWidgetIndex(int32(EPausePage::Settings)); }

#undef LOCTEXT_NAMESPACE
