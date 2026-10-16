// ENTROPÍA · Paso Inverso (rama Inverso, nivel 2)
// Mientras se mantiene la tecla, el personaje regresa en el tiempo usando su Registro Temporal.
// Gasta energía, desactiva el movimiento, distorsiona la cámara y reproduce la animación al revés.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InverseStepComponent.generated.h"

class UTemporalRecorderComponent;
class UCameraComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Inverse Step"))
class ENTROPY_API UInverseStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInverseStepComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MaxEnergy = 100.f;

	/** Energía por segundo mientras rebobinas (100/34 ≈ 3 s de rebobinado). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float DrainPerSecond = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float RegenPerSecond = 20.f;

	/** Segundos sin usar el poder antes de que la energía empiece a recargarse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float RegenDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MinEnergyToStart = 15.f;

	/** Grados extra de campo de visión mientras rebobinas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float FovKick = 12.f;

	/** Mostrar la barra de energía en pantalla (prototipo, sin UMG). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bShowDebugHud = true;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	float Energy = 100.f;

	/** Segundos entre cada copia de la estela mientras rebobinas (0 = sin estela). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	float AfterimageInterval = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	float AfterimageLife = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor AfterimageColor = FLinearColor(0.3f, 0.6f, 1.f, 0.35f);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void BeginInverseStep();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void EndInverseStep();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsInverseActive() const { return bActive; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyCameraEffect(float Alpha);
	void ApplyReversePose();
	void SetPoseProxyVisible(bool bVisible);
	void SpawnAfterimage();

	UPROPERTY()
	TObjectPtr<UTemporalRecorderComponent> Recorder;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	/** Copia "marioneta" del cuerpo que muestra la pose grabada mientras rebobinas. */
	UPROPERTY()
	TObjectPtr<UPoseableMeshComponent> PoseProxy;

	bool bActive = false;
	float EffectAlpha = 0.f;
	float BaseFov = 90.f;
	float TimeSinceUse = 0.f;
	float AfterimageTimer = 0.f;
};
