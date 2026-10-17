// ENTROPÍA · Dash (Fase 3)
// Shift: un impulso corto en la dirección en que te mueves (o hacia donde miras si estás quieto). También en el aire,
// una vez por salto. No gasta energía; tiene recarga. Deja estela y una onda chica de distorsión.
// La animación (AM_Dash de la plantilla) no mueve al personaje: la velocidad la pone este componente, así el dash
// va siempre hacia donde quieres aunque estés apuntando hacia otro lado.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalDashComponent.generated.h"

class UAnimMontage;
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
