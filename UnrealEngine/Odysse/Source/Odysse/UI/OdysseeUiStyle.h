// Direction visuelle de l'interface : palette, polices et pinceaux low-poly (textures générées par
// tools/ui/generate_ui_art.py, importées par tools/ui/import_ui_art.py).

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UTexture2D;

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

	/** Texture d'interface /Game/Odyssee/UI/Textures/T_Ui_<Name> (nullptr si absente). */
	UTexture2D* Texture(const TCHAR* Name);

	/** Image étirable par ses bords (coins coupés intacts) : panneaux, boutons, barres. */
	FSlateBrush BoxBrush(const TCHAR* Name, const FMargin& Margin);

	/** Image affichée telle quelle, à la taille demandée. */
	FSlateBrush ImageBrush(const TCHAR* Name, const FVector2D& Size);

	FSlateFontInfo Font(int32 Size, bool bBold = false, int32 LetterSpacing = 0);

	FButtonStyle ButtonStyle();
	FProgressBarStyle BarStyle();
}
