// ENTROPÍA · Dash (Fase 3)
// Shift: un impulso corto en la dirección en que te mueves (o hacia donde miras si estás quieto). También en el aire,
// una vez por salto. No gasta energía; tiene recarga. Deja estela y una onda chica de distorsión.
// La animación (AM_Dash de la plantilla) no mueve al personaje: la velocidad la pone este componente, así el dash
// va siempre hacia donde quieres aunque estés apuntando hacia otro lado.
// Esquiva perfecta: si al hacer dash una bala te iba a pegar en menos de DodgeWindow, entras en "tiempo de reflejo":
// el mundo va lento un momento y tú normal (se lo pide al árbitro del tiempo). Es un premio: no gasta energía.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalDashComponent.generated.h"

class UAnimMontage;
class UPostProcessComponent;
class UTemporalTrailComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Dash"))
class ENTROPY_API UTemporalDashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalDashComponent();

	/** cm/s durante el dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "100"))
	float DashSpeed = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.05"))
	float DashSeconds = 0.18f;

	/** Segundos entre un dash y el siguiente (contados desde que empieza). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float Cooldown = 0.6f;

	/** Segundos en que las balas te atraviesan al empezar el dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float InvulnerableSeconds = 0.25f;

	/** Si una bala te iba a pegar en menos de esto (segundos) al hacer dash, es esquiva perfecta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Reflejo", meta = (ClampMin = "0"))
	float DodgeWindow = 0.35f;

	/** Velocidad del mundo durante el tiempo de reflejo (tú vas normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Reflejo", meta = (ClampMin = "0.05", ClampMax = "1"))
	float ReflexDilation = 0.3f;

	/** Segundos reales que dura el tiempo de reflejo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Reflejo", meta = (ClampMin = "0.1"))
	float ReflexSeconds = 1.5f;

	/** Segundos entre cada copia de la estela durante el dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	float TrailInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor TrailColor = FLinearColor(0.85f, 0.95f, 1.f, 0.22f);

	/** False si está en recarga, ya usaste el del aire, o estás en el Paso Inverso. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool TryDash();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsDashing() const { return bDashing; }

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsReflexActive() const { return ReflexTimer > 0.f; }

	/** 0..1 de lo que queda del tiempo de reflejo (para el HUD). */
	float GetReflexAlpha() const { return ReflexSeconds > 0.f ? FMath::Clamp(ReflexTimer / ReflexSeconds, 0.f, 1.f) : 0.f; }

	/** El mundo va lento ReflexSeconds y tú normal. Lo llama la esquiva perfecta (y el modo tour). */
	void StartReflex();

	/** Corta el dash en seco (sin velocidad horizontal): al rematar o al morir. */
	void CancelDash();

	/** Corta el tiempo de reflejo (al morir). */
	void CancelReflex() { EndReflex(); }

	/** Dirección del dash: hacia donde te mueves (plano y normalizado) o, si estás quieto, hacia donde miras. */
	static FVector ComputeDashDirection(const FVector& MoveInput, const FVector& Facing);

	/** ¿Se puede hacer dash? Recarga cumplida y, en el aire, solo uno por salto. */
	static bool CanDash(float SinceLastDash, float InCooldown, bool bFalling, bool bAirDashUsed);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void EndDash();
	void EmitTrail();
	/** ¿Alguna bala que te puede pegar llegaría en menos de DodgeWindow? */
	bool IsPerfectDodge() const;
	void EndReflex();

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> ReflexPostProcess;

	float ReflexTimer = 0.f;
	float ReflexBlend = 0.f;

	UPROPERTY()
	TObjectPtr<UTemporalTrailComponent> Trail;

	UPROPERTY()
	TObjectPtr<UAnimMontage> DashMontage;

	FVector DashDirection = FVector::ForwardVector;
	float DashTimer = 0.f;
	float SinceLastDash = 100.f;
	float TrailTimer = 0.f;
	bool bDashing = false;
	bool bAirDashUsed = false;

	// Lo que el dash cambia del movimiento y devuelve al terminar.
	float SavedWalkSpeed = 500.f;
	float SavedGroundFriction = 8.f;
	float SavedBrakingWalking = 2000.f;
	float SavedBrakingFalling = 1500.f;
	float SavedGravityScale = 1.f;
};
