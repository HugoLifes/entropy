// ENTROPÍA · Locomoción (Fase 3b)
// Correr (mantener Shift), agacharse (C), el peso del personaje (aceleración, giro, cámara) y las caídas.
// Es el único que escribe la velocidad de caminar (MaxWalkSpeed) salvo durante el dash.
// Un hombre de casi 200 años: se mueve con peso, no como acróbata de caricatura.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaFalls.h"
#include "EntropiaLocomotionComponent.generated.h"

class ACharacter;
class UTemporalAimComponent;
struct FHitResult;

/** El último aterrizaje (lo lee el modo tour). */
struct FEntropiaLandingInfo
{
	EEntropiaLanding Kind = EEntropiaLanding::Normal;
	float Height = 0.f;
	float Damage = 0.f;
	int32 Count = 0;
};
class UTemporalDashComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Locomotion"))
class ENTROPY_API UEntropiaLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaLocomotionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Velocidad") float WalkSpeed = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Velocidad") float SprintSpeed = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Velocidad") float CrouchSpeed = 220.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Velocidad") float AimSpeed = 320.f;

	/** Grados por segundo al girar hacia donde caminas (peso). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Peso") float TurnRate = 360.f;

	/** Aceleración: menor = arranca con más peso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Peso") float Acceleration = 1500.f;

	/** Media altura de la cápsula agachado (de pie: 96). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Velocidad") float CrouchedHalfHeight = 60.f;

	/** Segundos que dura rodar al caer de 4 a 9 m y velocidad con la que ruedas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Caidas") float RollSeconds = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Caidas") float RollSpeed = 450.f;

	/** Segundos sin poder moverte tras un aterrizaje duro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Caidas") float HardLandingSeconds = 0.5f;

	/** Velocidad de caminar según el estado. Apuntar gana a correr; agachado gana a todo. Lógica pura (probada). */
	static float ComputeWalkSpeed(bool bSprint, bool bCrouch, float AimAlpha,
		float InWalk = 450.f, float InSprint = 700.f, float InCrouch = 220.f, float InAim = 320.f);

	/** Shift mantenido. Correr te levanta si estabas agachado. */
	void SetSprint(bool bSprint);
	bool IsSprinting() const { return bWantsSprint; }

	void ToggleCrouch();
	void SetCrouched(bool bCrouch);
	bool IsCrouched() const;

	/** Otra pose para cuando estás agachado (la de cobertura baja). Nulo = la normal (CrouchPose). */
	void SetCrouchPoseOverride(const FEntropiaAnimSlot* Pose);

	bool IsRolling() const { return RollTimer > 0.f; }
	bool IsHardLanding() const { return HardTimer > 0.f; }
	const FEntropiaLandingInfo& GetLastLanding() const { return LastLanding; }

	/** Olvida la altura desde la que vienes cayendo (al teletransportarte). */
	void ResetFall();

	/** Modo tour: simula que el jugador mantiene el stick en esta dirección (cero = soltar). */
	void SetTourInput(const FVector& WorldDirection) { TourInput = WorldDirection; }
	FVector GetTourInput() const { return TourInput; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	ACharacter* GetCharacter() const;
	void ApplyCrouchPose();
	void TrackFall();
	void StartRoll(const FVector& Direction);
	void UpdateRoll(float DeltaTime);
	void EndRoll();

	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> Aim;

	UPROPERTY()
	TObjectPtr<UTemporalDashComponent> Dash;

	FEntropiaLandingInfo LastLanding;
	float ApexZ = 0.f;
	bool bWasFalling = false;
	FVector LastLocation = FVector::ZeroVector;
	float RollTimer = 0.f;
	float HardTimer = 0.f;
	FVector RollDirection = FVector::ForwardVector;
	FVector MeshBaseLocation = FVector::ZeroVector;
	FRotator MeshBaseRotation = FRotator::ZeroRotator;

	bool bWantsSprint = false;
	bool bCrouchPosePlaying = false;
	/** La pose de agachado que se puso la última vez (para quitarla o cambiarla por otra). */
	FEntropiaAnimSlot PlayingCrouchPose;
	TOptional<FEntropiaAnimSlot> CrouchPoseOverride;
	FVector TourInput = FVector::ZeroVector;
};
