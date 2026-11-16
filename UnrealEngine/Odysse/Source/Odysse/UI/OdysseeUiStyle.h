// Direction visuelle de l'interface : palette, polices et pinceaux low-poly (textures générées par
// tools/ui/generate_ui_art.py, importées par tools/ui/import_ui_art.py).

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UTexture2D;

/** Deux habillages pour les mêmes écrans : sombre et facetté (HUD, menus), ou hologramme cyan
 *  (le bracelet de stase, docs/BRACELET.md). */
enum class EOdysseeUiTheme : uint8
{
	Ground,
	Holo,
};

namespace OdysseeUi
{
	// Palette : tons de la plaine (mousse, terre, os) et un accent rouille pour le survol.
	inline const FLinearColor Bone(0.93f, 0.89f, 0.80f);
	inline const FLinearColor BoneDim(0.93f, 0.89f, 0.80f, 0.65f);
	inline const FLinearColor Rust(0.92f, 0.62f, 0.27f);
	inline const FLinearColor Shade(0.01f, 0.012f, 0.01f, 0.55f);
	inline const FLinearColor Health(0.80f, 0.27f, 0.22f);
	inline const FLinearColor Stamina(0.88f, 0.74f, 0.32f);
	inline const FLinearColor Hunger(0.88f, 0.55f, 0.25f);
	inline const FLinearColor Thirst(0.38f, 0.66f, 0.88f);

	// Hologramme : cyan clair, accent blanc bleuté.
	inline const FLinearColor HoloText(0.80f, 0.96f, 1.0f);
	inline const FLinearColor HoloDim(0.55f, 0.85f, 0.95f, 0.75f);
	inline const FLinearColor HoloAccent(0.55f, 0.95f, 1.0f);

	/** Texture d'interface /Game/Odyssee/UI/Textures/T_Ui_<Name> (nullptr si absente). En thème
	 *  hologramme, T_Ui_holo_<Name> si elle existe (sinon la même : les icônes ne changent pas). */
	UTexture2D* Texture(const TCHAR* Name, EOdysseeUiTheme Theme = EOdysseeUiTheme::Ground);

	/** Image étirable par ses bords (coins coupés intacts) : panneaux, boutons, barres. */
	FSlateBrush BoxBrush(const TCHAR* Name, const FMargin& Margin, EOdysseeUiTheme Theme = EOdysseeUiTheme::Ground);

	/** Image affichée telle quelle, à la taille demandée. */
	FSlateBrush ImageBrush(const TCHAR* Name, const FVector2D& Size, EOdysseeUiTheme Theme = EOdysseeUiTheme::Ground);

	FSlateFontInfo Font(int32 Size, bool bBold = false, int32 LetterSpacing = 0);

	FButtonStyle ButtonStyle(EOdysseeUiTheme Theme = EOdysseeUiTheme::Ground);
	FProgressBarStyle BarStyle(EOdysseeUiTheme Theme = EOdysseeUiTheme::Ground);

	/** Couleurs du texte selon le thème : normal, discret, accent (titres, choix). */
	FLinearColor TextColor(EOdysseeUiTheme Theme);
	FLinearColor DimColor(EOdysseeUiTheme Theme);
	FLinearColor AccentColor(EOdysseeUiTheme Theme);

	/** Thème des écrans créés pendant sa durée de vie (un écran le transmet à ses sous-écrans). */
	struct FScopedTheme
	{
		explicit FScopedTheme(EOdysseeUiTheme Theme);
		~FScopedTheme();

	private:
		EOdysseeUiTheme Previous;
	};
	EOdysseeUiTheme CurrentTheme();
}
