#include "UI/OdysseeUiStyle.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace OdysseeUi
{
	namespace
	{
		const FMargin ButtonMargin(16.f / 256.f, 16.f / 64.f);
		const FMargin BarMargin(8.f / 256.f, 0.3f);
		EOdysseeUiTheme Current = EOdysseeUiTheme::Ground;

		UTexture2D* Load(const FString& Name)
		{
			const FString Path = FString::Printf(TEXT("/Game/Odyssee/UI/Textures/T_Ui_%s.T_Ui_%s"), *Name, *Name);
			return LoadObject<UTexture2D>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}

	UTexture2D* Texture(const TCHAR* Name, EOdysseeUiTheme Theme)
	{
		if (Theme == EOdysseeUiTheme::Holo)
		{
			// Cherchée une fois par nom : la plupart des textures (icônes) n'ont pas de version holo.
			static TMap<FString, TWeakObjectPtr<UTexture2D>> HoloFound;
			static TSet<FString> HoloMissing;
			const FString Key(Name);
			if (!HoloMissing.Contains(Key))
			{
				if (const TWeakObjectPtr<UTexture2D>* Known = HoloFound.Find(Key); Known && Known->IsValid())
				{
					return Known->Get();
				}
				if (UTexture2D* Holo = Load(TEXT("holo_") + Key))
				{
					HoloFound.Add(Key, Holo);
					return Holo;
				}
				HoloMissing.Add(Key);
			}
		}
		return Load(Name);
	}

	FSlateBrush BoxBrush(const TCHAR* Name, const FMargin& Margin, EOdysseeUiTheme Theme)
	{
		FSlateBrush Brush;
		if (UTexture2D* Image = Texture(Name, Theme))
		{
			Brush.SetResourceObject(Image);
			Brush.ImageSize = FVector2D(Image->GetSizeX(), Image->GetSizeY());
		}
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = Margin;
		return Brush;
	}

	FSlateBrush ImageBrush(const TCHAR* Name, const FVector2D& Size, EOdysseeUiTheme Theme)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture(Name, Theme));
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	FSlateFontInfo Font(int32 Size, bool bBold, int32 LetterSpacing)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}

	FButtonStyle ButtonStyle(EOdysseeUiTheme Theme)
	{
		FButtonStyle Style;
		Style.SetNormal(BoxBrush(TEXT("bouton"), ButtonMargin, Theme));
		Style.SetHovered(BoxBrush(TEXT("bouton_survol"), ButtonMargin, Theme));
		Style.SetPressed(BoxBrush(TEXT("bouton_appui"), ButtonMargin, Theme));
		Style.SetDisabled(BoxBrush(TEXT("bouton_grise"), ButtonMargin, Theme));
		Style.SetNormalPadding(FMargin(18.f, 8.f));
		Style.SetPressedPadding(FMargin(18.f, 10.f, 18.f, 6.f));
		return Style;
	}

	FProgressBarStyle BarStyle(EOdysseeUiTheme Theme)
	{
		FProgressBarStyle Style;
		Style.SetBackgroundImage(BoxBrush(TEXT("barre_fond"), BarMargin, Theme));
		Style.SetFillImage(BoxBrush(TEXT("barre_plein"), BarMargin, Theme));
		Style.SetMarqueeImage(BoxBrush(TEXT("barre_plein"), BarMargin, Theme));
		return Style;
	}

	FLinearColor TextColor(EOdysseeUiTheme Theme)
	{
		return Theme == EOdysseeUiTheme::Holo ? HoloText : Bone;
	}

	FLinearColor DimColor(EOdysseeUiTheme Theme)
	{
		return Theme == EOdysseeUiTheme::Holo ? HoloDim : BoneDim;
	}

	FLinearColor AccentColor(EOdysseeUiTheme Theme)
	{
		return Theme == EOdysseeUiTheme::Holo ? HoloAccent : Rust;
	}

	FScopedTheme::FScopedTheme(EOdysseeUiTheme Theme) : Previous(Current)
	{
		Current = Theme;
	}

	FScopedTheme::~FScopedTheme()
	{
		Current = Previous;
	}

	EOdysseeUiTheme CurrentTheme()
	{
		return Current;
	}
}
