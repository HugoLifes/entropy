// ENTROPÍA · Locomoción (Fase 3b)
// Correr (mantener Shift), agacharse (C), el peso del personaje (aceleración, giro, cámara) y las caídas.
// Es el único que escribe la velocidad de caminar (MaxWalkSpeed) salvo durante el dash.
// Un hombre de casi 200 años: se mueve con peso, no como acróbata de caricatura.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaLocomotionComponent.generated.h"

class ACharacter;
class UTemporalAimComponent;
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

	/** Velocidad de caminar según el estado. Apuntar gana a correr; agachado gana a todo. Lógica pura (probada). */
	static float ComputeWalkSpeed(bool bSprint, bool bCrouch, float AimAlpha,
		float InWalk = 450.f, float InSprint = 700.f, float InCrouch = 220.f, float InAim = 320.f);

	/** Shift mantenido. Correr te levanta si estabas agachado. */
	void SetSprint(bool bSprint);
	bool IsSprinting() const { return bWantsSprint; }

	void ToggleCrouch();
	void SetCrouched(bool bCrouch);
	bool IsCrouched() const;

	/** Modo tour: simula que el jugador mantiene el stick en esta dirección (cero = soltar). */
	void SetTourInput(const FVector& WorldDirection) { TourInput = WorldDirection; }
	FVector GetTourInput() const { return TourInput; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	ACharacter* GetCharacter() const;
	void ApplyCrouchPose();

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> Aim;

	UPROPERTY()
	TObjectPtr<UTemporalDashComponent> Dash;

	bool bWantsSprint = false;
	bool bCrouchPosePlaying = false;
	FVector TourInput = FVector::ZeroVector;
};
