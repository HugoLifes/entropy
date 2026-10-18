// ENTROPÍA · Apuntar y mira (Fase 3)
// La cámara va sobre el hombro derecho. Clic derecho mantenido = apuntar: la cámara se acerca, caminas más lento
// y el personaje mira hacia donde apuntas.
// La mira (el centro de la pantalla) SELECCIONA el objeto sobre el que actúan los poderes (R invierte, clic izquierdo
// detiene): lo que tiene Registro Temporal y los "objetivos de tiempo" (maniquíes, torretas). Lo seleccionado se
// resalta en ámbar; si tiene grupo (los pedazos de un mismo objeto roto), se resalta el grupo entero.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalAimComponent.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;
class USpringArmComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Aim"))
class ENTROPY_API UTemporalAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalAimComponent();

	/** Distancia de la cámara sin apuntar y apuntando (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Camara")
	float ArmLength = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Camara")
	float AimArmLength = 150.f;

	/** Corrimiento de la cámara (Y = a la derecha del personaje, Z = arriba). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Camara")
	FVector ShoulderOffset = FVector(0.f, 55.f, 45.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Camara")
	FVector AimShoulderOffset = FVector(0.f, 65.f, 55.f);

	/** Qué tan rápido entra y sale la cámara al apuntar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Camara")
	float BlendSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float AimWalkSpeed = 320.f;

	/** Grados por segundo que gira el personaje para mirar hacia donde apuntas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float AimTurnRate = 720.f;

	/** Qué tan lejos de la mira puede estar un objeto (grados). Los objetos grandes cuentan desde su borde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float MaxAngleDegrees = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "100"))
	float MaxDistance = 3500.f;

	/** Las balas se seleccionan primero si están a menos de estos grados de la mira (aunque haya algo grande detrás). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float BulletAngleDegrees = 6.f;

	/** Las balas ya incrustadas (con armas hay muchas) solo ganan si apuntas casi exacto a ellas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.1"))
	float EmbeddedBulletAngleDegrees = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor HighlightColor = FLinearColor(1.f, 0.65f, 0.15f, 0.35f);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void SetAiming(bool bAim);

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsAiming() const { return bAiming; }

	/** 0 = sin apuntar, 1 = apuntando (sube y baja suave). */
	float GetAimAlpha() const { return AimAlpha; }

	/** Hombro de la cámara: +1 derecho (normal), -1 izquierdo (al asomarte por una esquina izquierda). */
	void SetShoulderSide(float Side) { ShoulderTarget = Side >= 0.f ? 1.f : -1.f; }

	/** La cámara se acerca un momento (golpe con enfoque). */
	void StartFocus(float Seconds) { FocusTimer = Seconds; }

	/** Busca de nuevo lo que está en la mira (la cámara pudo girar desde el último frame). */
	AActor* FindTarget() const;

	/** Lo seleccionado en este frame (lo resaltado). */
	AActor* GetHoveredTarget() const { return Hovered.Get(); }

	/** Fija la selección (por ejemplo, mientras inviertes algo, aunque la mira se mueva). */
	void LockTarget(AActor* Target);
	void UnlockTarget();

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
	void Select(AActor* Target);
	void ClearHighlight();
	void ApplyCamera();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HighlightMaterial;

	UPROPERTY()
	TObjectPtr<USpringArmComponent> Boom;

	TWeakObjectPtr<AActor> Hovered;
	TWeakObjectPtr<AActor> Locked;
	TArray<TWeakObjectPtr<UMeshComponent>> Highlighted;

	bool bAiming = false;
	float AimAlpha = 0.f;
	float ShoulderSide = 1.f;
	float ShoulderTarget = 1.f;
	float FocusTimer = 0.f;
	float FocusAlpha = 0.f;
	float BaseWalkSpeed = 500.f;
	bool bBaseOrientToMovement = true;
	bool bBaseUseControllerDesiredRotation = false;
	FRotator BaseRotationRate = FRotator(0.f, 500.f, 0.f);
};
