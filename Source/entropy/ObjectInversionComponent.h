// ENTROPÍA · Invertir objeto (rama Inverso, nivel 1) — Fase 1
// Apuntas con el centro de la pantalla: el objeto con Registro Temporal más cercano a la mira se resalta.
// Mantienes R: SOLO ese objeto (y su grupo: los pedazos de un mismo objeto roto, o una bala con lo que rompió)
// regresa en el tiempo. Gasta energía mientras lo mantienes.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectInversionComponent.generated.h"

class UTemporalRecorderComponent;
class UTemporalEnergyComponent;
class UMaterialInstanceDynamic;
class UMeshComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Object Inversion"))
class ENTROPY_API UObjectInversionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UObjectInversionComponent();

	/** Qué tan lejos de la mira puede estar un objeto (grados). Los objetos grandes cuentan desde su borde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float MaxAngleDegrees = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "100"))
	float MaxDistance = 3500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float CostPerSecond = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MinToStart = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor HighlightColor = FLinearColor(1.f, 0.65f, 0.15f, 0.35f);

	/** Empieza a rebobinar lo que apuntas. False (y la barra parpadea) si no hay objetivo o energía. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool BeginInversion();

	/** Como BeginInversion, pero con un objetivo dado (sin apuntar). Lo usa el modo tour. */
	bool BeginInversionOn(UTemporalRecorderComponent* Target);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void EndInversion();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsInverting() const { return bInverting; }

	/**
	 * ¿El candidato A está mejor apuntado que B? EdgeAngle = grados de la mira al borde del objeto
	 * (negativo si el objeto cubre la mira); CenterAngle = grados de la mira a su centro.
	 */
	static bool IsBetterAim(float EdgeAngleA, float CenterAngleA, float EdgeAngleB, float CenterAngleB);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UTemporalRecorderComponent* FindTarget() const;
	/** Empieza a rebobinar el grupo de Target (sin buscar con la mira). */
	bool StartInversionOn(UTemporalRecorderComponent* Target);
	void HighlightGroupOf(UTemporalRecorderComponent* Target);
	void ClearHighlight();

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HighlightMaterial;

	TWeakObjectPtr<UTemporalRecorderComponent> CurrentTarget;
	TArray<TWeakObjectPtr<UTemporalRecorderComponent>> ActiveGroup;
	TArray<TWeakObjectPtr<UMeshComponent>> Highlighted;
	bool bInverting = false;
};
