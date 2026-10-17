// ENTROPÍA · Paso Inverso (rama Inverso, nivel 2)
// Mientras se mantiene la tecla, el personaje regresa en el tiempo usando su Registro Temporal.
// Gasta energía compartida, desactiva el movimiento, distorsiona la imagen y reproduce la animación al revés.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InverseStepComponent.generated.h"

class UTemporalRecorderComponent;
class UTemporalEnergyComponent;
class UTemporalTrailComponent;
class UCameraComponent;
class UPostProcessComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Inverse Step"))
class ENTROPY_API UInverseStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInverseStepComponent();

	/** Energía por segundo mientras rebobinas (100/34 ≈ 3 s de rebobinado). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float CostPerSecond = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float MinToStart = 15.f;

	/** Al soltar, sales con la velocidad que llevabas en ese instante del pasado (1 = toda, 0 = te frenas en seco). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.0", ClampMax = "1.5"))
	float ExitMomentumScale = 1.f;

	/** Grados extra de campo de visión mientras rebobinas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float FovKick = 12.f;

	/** Texto de debug con la energía (ya hay barra de UMG; queda para depurar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bShowDebugHud = false;

	/** Segundos entre cada copia de la estela mientras rebobinas (0 = sin estela). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	float AfterimageInterval = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor AfterimageColor = FLinearColor(0.25f, 0.45f, 0.95f, 0.22f);

	/** Empieza a regresar en el tiempo. Devuelve false si no pudo (sin energía o sin grabación). */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool BeginInverseStep();

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
	void EmitAfterimage();

	UPROPERTY()
	TObjectPtr<UTemporalRecorderComponent> Recorder;

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UTemporalTrailComponent> Trail;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	/** Post-process propio del poder: así no pisa el de la Sobremarcha ni el de la cámara. */
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	/** Copia "marioneta" del cuerpo que muestra la pose grabada mientras rebobinas. */
	UPROPERTY()
	TObjectPtr<UPoseableMeshComponent> PoseProxy;

	bool bActive = false;
	float EffectAlpha = 0.f;
	float BaseFov = 90.f;
	float AfterimageTimer = 0.f;
};
