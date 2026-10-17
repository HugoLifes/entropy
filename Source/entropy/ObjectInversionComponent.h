// ENTROPÍA · Invertir objeto (rama Inverso, nivel 1) — Fase 1
// Lo que selecciona la mira (UTemporalAimComponent) es el objetivo. Mantienes R: SOLO ese objeto (y su grupo: los
// pedazos de un mismo objeto roto, o una bala con lo que rompió) regresa en el tiempo. Gasta energía mientras lo mantienes.
// Mientras inviertes, la selección se queda fija en él aunque muevas la mira.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectInversionComponent.generated.h"

class UTemporalRecorderComponent;
class UTemporalEnergyComponent;
class UTemporalAimComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Object Inversion"))
class ENTROPY_API UObjectInversionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UObjectInversionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float CostPerSecond = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MinToStart = 10.f;

	/** Empieza a rebobinar lo que apuntas. False (y la barra parpadea) si no hay objetivo o energía. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool BeginInversion();

	/** Como BeginInversion, pero con un objetivo dado (sin apuntar). Lo usa el modo tour. */
	bool BeginInversionOn(UTemporalRecorderComponent* Target);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void EndInversion();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsInverting() const { return bInverting; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Empieza a rebobinar el grupo de Target (sin buscar con la mira). */
	bool StartInversionOn(UTemporalRecorderComponent* Target);

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> Aim;

	TArray<TWeakObjectPtr<UTemporalRecorderComponent>> ActiveGroup;
	bool bInverting = false;
};
