// ENTROPÍA · Sobremarcha (rama Brecha) — Fase 1
// V prende y apaga: el mundo va lento (dilatación global) y tú a velocidad normal (tu dilatación la compensa).
// Deja una estela cian con tu pose y tiñe la imagen. Gasta energía en tiempo real.
// No va junto con el Paso Inverso: encender uno apaga el otro.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OverdriveComponent.generated.h"

class UTemporalEnergyComponent;
class UTemporalTrailComponent;
class UPostProcessComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Overdrive (Sobremarcha)"))
class ENTROPY_API UOverdriveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOverdriveComponent();

	/** Qué tan lento va el mundo (0.25 = 4 veces más lento). Tú vas a 1/WorldDilation para compensar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.05", ClampMax = "1"))
	float WorldDilation = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float CostPerSecond = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MinToStart = 20.f;

	/** Segundos reales entre cada copia de la estela. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	float TrailInterval = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor TrailColor = FLinearColor(0.2f, 0.9f, 0.8f, 0.2f);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Toggle();

	/** False (y la barra parpadea) si no alcanza la energía. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool StartOverdrive();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void StopOverdrive();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsOverdriveActive() const { return bOverdrive; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void EmitTrail();

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UTemporalTrailComponent> Trail;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcess;

	bool bOverdrive = false;
	float EffectAlpha = 0.f;
	float TrailTimer = 0.f;
};
