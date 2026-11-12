#include "UI/OdysseeOptionsWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/OdysseeUiStyle.h"

#define LOCTEXT_NAMESPACE "OdysseeOptions"

namespace
{
	struct FBinding
	{
		const TCHAR* Action;
		const TCHAR* Key;
	};

	// Touches par défaut (AZERTY), comme dans docs/GAMEPLAY.md.
	const FBinding DefaultBindings[] = {
		{TEXT("Avancer"), TEXT("Z")}, {TEXT("Reculer"), TEXT("S")}, {TEXT("Aller à gauche"), TEXT("Q")},
		{TEXT("Aller à droite"), TEXT("D")}, {TEXT("Courir"), TEXT("Maj")}, {TEXT("Sauter"), TEXT("Espace")},
		{TEXT("Interagir, ramasser"), TEXT("E")}, {TEXT("Attaquer"), TEXT("Clic gauche")},
		{TEXT("Bloquer, viser"), TEXT("Clic droit")}, {TEXT("Esquiver"), TEXT("Ctrl")},
		{TEXT("Outils 1 à 5"), TEXT("1 – 5, molette")}, {TEXT("Inventaire"), TEXT("Tab")},
		{TEXT("Carte"), TEXT("M")}, {TEXT("Journal d'indices"), TEXT("J")}, {TEXT("Lampe torche"), TEXT("F")},
		{TEXT("Pause"), TEXT("Échap")},
	};
}

UWidget* UOdysseeKeyBindingsWidget::BuildTree()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("KeysTitle", "Touches"), Column);
	BindingList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BindingList"));
	int32 Index = 0;
	for (const FBinding& Binding : DefaultBindings)
	{
		BindingList->AddChild(BuildRow(++Index, FText::FromString(Binding.Action), FText::FromString(Binding.Key)));
	}
	AddTo(Column, MakeSized(BindingList, FVector2D(520.f, 430.f)), FMargin(0.f, 0.f, 0.f, 12.f));
	BackButton = MakeButton(TEXT("BackButton"), LOCTEXT("Back", "Retour"));
	AddButton(Column, BackButton);
	return Panel;
}

UWidget* UOdysseeKeyBindingsWidget::BuildRow(int32 Index, const FText& Action, const FText& Key)
{
	UHorizontalBox* Row = MakeRow();
	UTextBlock* Label = MakeText(*FString::Printf(TEXT("ActionText%d"), Index), Action, 14);
	AddTo(Row, MakeSized(Label, FVector2D(300.f, 40.f)), FMargin(4.f, 2.f));
	UButton* KeyButton = MakeButton(*FString::Printf(TEXT("KeyButton%d"), Index), Key, 190.f);
	KeyButtons.Add(KeyButton);
	AddTo(Row, KeyButton->GetParent(), FMargin(4.f, 2.f));
	return Row;
}

UWidget* UOdysseeSettingsWidget::BuildTree()
{
	UVerticalBox* Column = nullptr;
	UBorder* Panel = MakeMenuPanel(NAME_None, LOCTEXT("SettingsTitle", "Paramètres"), Column);
	MouseSensitivitySlider = MakeSlider(TEXT("MouseSensitivitySlider"), 0.5f);
	VolumeSlider = MakeSlider(TEXT("VolumeSlider"), 0.8f);
	GraphicsPresetCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),
		TEXT("GraphicsPresetCombo"));
	for (const TCHAR* Preset : {TEXT("Bas"), TEXT("Moyen"), TEXT("Haut"), TEXT("Ultra")})
	{
		GraphicsPresetCombo->AddOption(Preset);
	}
	GraphicsPresetCombo->SetSelectedOption(TEXT("Haut"));
	AddTo(Column, BuildSettingRow(LOCTEXT("Mouse", "Sensibilité de la souris"), MouseSensitivitySlider));
	AddTo(Column, BuildSettingRow(LOCTEXT("Volume", "Volume général"), VolumeSlider));
	AddTo(Column, BuildSettingRow(LOCTEXT("Graphics", "Qualité graphique"), GraphicsPresetCombo),
		FMargin(0.f, 4.f, 0.f, 18.f));
	KeyBindingsButton = MakeButton(TEXT("KeyBindingsButton"), LOCTEXT("Keys", "Modifier les touches"));
	ApplyButton = MakeButton(TEXT("ApplyButton"), LOCTEXT("Apply", "Appliquer"));
	BackButton = MakeButton(TEXT("BackButton"), LOCTEXT("Back", "Retour"));
	AddButton(Column, KeyBindingsButton);
	AddButton(Column, ApplyButton);
	AddButton(Column, BackButton);
	return Panel;
}

UWidget* UOdysseeSettingsWidget::BuildSettingRow(const FText& Label, UWidget* Control)
{
	UHorizontalBox* Row = MakeRow();
	AddTo(Row, MakeSized(MakeText(NAME_None, Label, 14), FVector2D(250.f, 40.f)), FMargin(4.f, 2.f));
	AddTo(Row, MakeSized(Control, FVector2D(240.f, 32.f)), FMargin(4.f, 2.f));
	return Row;
}

USlider* UOdysseeSettingsWidget::MakeSlider(FName Name, float Value)
{
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
	Slider->SetValue(Value);
	Slider->SetSliderBarColor(OdysseeUi::BoneDim);
	Slider->SetSliderHandleColor(OdysseeUi::Rust);
	return Slider;
}

#undef LOCTEXT_NAMESPACE
