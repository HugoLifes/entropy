// ENTROPÍA · Barra de energía

#include "TemporalHUDWidget.h"
#include "TemporalEnergyComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace EntropiaHUD
{
	constexpr float BarWidth = 320.f;
	constexpr float BarHeight = 14.f;
	constexpr float BottomMargin = 40.f;

	FLinearColor PowerColor(FName Power)
	{
		if (Power == TEXT("Inverso")) { return FLinearColor(0.3f, 0.6f, 1.f); }
		if (Power == TEXT("Inversion")) { return FLinearColor(1.f, 0.65f, 0.15f); }
		if (Power == TEXT("Sobremarcha")) { return FLinearColor(0.2f, 0.9f, 0.8f); }
		return FLinearColor(0.9f, 0.9f, 0.9f);
	}
}

void UTemporalHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Solo dibuja: que no se trague los clics del ratón.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UTemporalHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const UTemporalEnergyComponent* EnergyComp = Energy.Get();
	if (!EnergyComp)
	{
		return LayerId;
	}

	using namespace EntropiaHUD;
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D BarPos((Size.X - BarWidth) * 0.5f, Size.Y - BottomMargin - BarHeight);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");

	// Fondo.
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth + 4.f, BarHeight + 4.f), FSlateLayoutTransform(BarPos - FVector2D(2.f, 2.f))),
		White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f));

	// Relleno: rojo si acabas de pedir un poder sin energía suficiente.
	const float Fill = FMath::Clamp(EnergyComp->GetFraction(), 0.f, 1.f);
	const FLinearColor FillColor = EnergyComp->GetDenyFlash() > 0.f ? FLinearColor(1.f, 0.15f, 0.1f) : PowerColor(EnergyComp->GetActivePower());
	if (Fill > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth * Fill, BarHeight), FSlateLayoutTransform(BarPos)),
			White, ESlateDrawEffect::None, FillColor);
	}

	// Etiqueta encima de la barra.
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth, 16.f), FSlateLayoutTransform(BarPos - FVector2D(0.f, 18.f))),
		FString(TEXT("ENERGÍA")), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.85f));

	return LayerId + 2;
}
