// ENTROPÍA · Barra de energía (Fase 1)
// UMG sin asset: se dibuja en NativePaint con cajas y texto de Slate. Abajo al centro, del color del poder activo;
// parpadea en rojo cuando pides un poder y no alcanza la energía.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TemporalHUDWidget.generated.h"

class UTemporalEnergyComponent;

UCLASS()
class ENTROPY_API UTemporalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEnergy(UTemporalEnergyComponent* InEnergy) { Energy = InEnergy; }

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TWeakObjectPtr<UTemporalEnergyComponent> Energy;
};
