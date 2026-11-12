#include "UI/OdysseeUiStyle.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace OdysseeUi
{
	namespace
	{
		const FMargin ButtonMargin(16.f / 256.f, 16.f / 64.f);
		const FMargin BarMargin(8.f / 256.f, 0.3f);
	}

	UTexture2D* Texture(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Odyssee/UI/Textures/T_Ui_%s.T_Ui_%s"), Name, Name);
		return LoadObject<UTexture2D>(nullptr, *Path);
	}

	FSlateBrush BoxBrush(const TCHAR* Name, const FMargin& Margin)
	{
		FSlateBrush Brush;
		if (UTexture2D* Image = Texture(Name))
		{
			Brush.SetResourceObject(Image);
			Brush.ImageSize = FVector2D(Image->GetSizeX(), Image->GetSizeY());
		}
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = Margin;
		return Brush;
	}

	FSlateBrush ImageBrush(const TCHAR* Name, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture(Name));
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

	FButtonStyle ButtonStyle()
	{
		FButtonStyle Style;
		Style.SetNormal(BoxBrush(TEXT("bouton"), ButtonMargin));
		Style.SetHovered(BoxBrush(TEXT("bouton_survol"), ButtonMargin));
		Style.SetPressed(BoxBrush(TEXT("bouton_appui"), ButtonMargin));
		Style.SetDisabled(BoxBrush(TEXT("bouton_grise"), ButtonMargin));
		Style.SetNormalPadding(FMargin(18.f, 8.f));
		Style.SetPressedPadding(FMargin(18.f, 10.f, 18.f, 6.f));
		return Style;
	}

	FProgressBarStyle BarStyle()
	{
		FProgressBarStyle Style;
		Style.SetBackgroundImage(BoxBrush(TEXT("barre_fond"), BarMargin));
		Style.SetFillImage(BoxBrush(TEXT("barre_plein"), BarMargin));
		Style.SetMarqueeImage(BoxBrush(TEXT("barre_plein"), BarMargin));
		return Style;
	}
}
