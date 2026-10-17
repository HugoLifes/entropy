// ENTROPÍA · Barra de energía

#include "TemporalHUDWidget.h"
#include "TemporalEnergyComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalDilation.h"
#include "TemporalHealthComponent.h"

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
		if (Power == TEXT("Detener")) { return FLinearColor(0.6f, 0.92f, 1.f); }
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

	PaintCrosshair(AllottedGeometry, OutDrawElements, LayerId + 3);

	const UTemporalEnergyComponent* EnergyComp = Energy.Get();
	if (!EnergyComp)
	{
		return LayerId + 3;
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

	// Vida: barra roja encima de la de energía; parpadea clara al recibir daño.
	if (const UTemporalHealthComponent* HealthComp = Health.Get())
	{
		const FVector2D HealthPos = BarPos - FVector2D(0.f, 42.f);
		const float HealthHeight = 10.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth + 4.f, HealthHeight + 4.f), FSlateLayoutTransform(HealthPos - FVector2D(2.f, 2.f))),
			White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		const float HealthFill = FMath::Clamp(HealthComp->GetFraction(), 0.f, 1.f);
		const FLinearColor HealthColor = HealthComp->GetHurtFlash() > 0.f ? FLinearColor(1.f, 0.75f, 0.7f) : FLinearColor(0.9f, 0.15f, 0.12f);
		if (HealthFill > 0.f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth * HealthFill, HealthHeight), FSlateLayoutTransform(HealthPos)),
				White, ESlateDrawEffect::None, HealthColor);
		}
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth, 16.f), FSlateLayoutTransform(HealthPos - FVector2D(0.f, 18.f))),
			FString(TEXT("VIDA")), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.85f));
	}

	return LayerId + 3;
}

void UTemporalHUDWidget::PaintCrosshair(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const UTemporalAimComponent* AimComp = Aim.Get();
	if (!AimComp)
	{
		return;
	}

	// Color según lo que hay en la mira.
	FLinearColor Color(1.f, 1.f, 1.f, 0.75f);
	if (const AActor* Target = AimComp->GetHoveredTarget())
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		Color = (Dilation && Dilation->IsFrozen(Target)) ? FLinearColor(0.55f, 0.95f, 1.f, 1.f) : FLinearColor(1.f, 0.65f, 0.15f, 1.f);
	}

	// Cuatro rayitas alrededor de un punto; al apuntar se cierran (más precisión).
	const float Alpha = AimComp->GetAimAlpha();
	const float TickGap = FMath::Lerp(10.f, 6.f, Alpha);
	const float TickLength = FMath::Lerp(8.f, 10.f, Alpha);
	const float TickThick = 2.f;
	const FVector2D Center = Geometry.GetLocalSize() * 0.5f;
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	auto Box = [&](const FVector2D& TopLeft, const FVector2D& Size, const FLinearColor& BoxColor)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft)), White, ESlateDrawEffect::None, BoxColor);
	};
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.45f * Color.A);
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		// Primero una sombra 1 px más grande (se ve sobre fondos claros), luego el color.
		const float Pad = Pass == 0 ? 1.f : 0.f;
		const FLinearColor& C = Pass == 0 ? Shadow : Color;
		Box(Center + FVector2D(-TickThick * 0.5f - Pad, -TickGap - TickLength - Pad), FVector2D(TickThick + Pad * 2.f, TickLength + Pad * 2.f), C);
		Box(Center + FVector2D(-TickThick * 0.5f - Pad, TickGap - Pad), FVector2D(TickThick + Pad * 2.f, TickLength + Pad * 2.f), C);
		Box(Center + FVector2D(-TickGap - TickLength - Pad, -TickThick * 0.5f - Pad), FVector2D(TickLength + Pad * 2.f, TickThick + Pad * 2.f), C);
		Box(Center + FVector2D(TickGap - Pad, -TickThick * 0.5f - Pad), FVector2D(TickLength + Pad * 2.f, TickThick + Pad * 2.f), C);
		Box(Center - FVector2D(1.f + Pad, 1.f + Pad), FVector2D(2.f + Pad * 2.f, 2.f + Pad * 2.f), C);
	}
}
