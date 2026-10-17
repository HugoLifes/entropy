// ENTROPÍA · Vida simple (Fase 3)
// Hace falta para que esquivar importe: 100 de vida, cada bala de torreta quita 20, se recupera sola tras 4 s sin
// daño. En 0 avisa (OnDeath) y el Sandbox te hace reaparecer. Durante el dash eres invulnerable un instante.
// Regresar en el tiempo (F) NO devuelve la vida (decisión de Hugo, por ahora).
// Se mide en tiempo real, como la energía.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnTemporalDeath);

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Health"))
class ENTROPY_API UTemporalHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Segundos sin daño antes de empezar a recuperarse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float RegenDelay = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float RegenPerSecond = 25.f;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	float Health = 100.f;

	/** Quita vida. False si eres invulnerable o ya estás muerto (no hizo nada). Al llegar a 0 avisa una sola vez. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool ApplyDamage(float Amount);

	/** Ignora el daño durante estos segundos (dash, reaparecer). */
	void SetInvulnerable(float Seconds) { InvulnerableTime = FMath::Max(InvulnerableTime, Seconds); }
	bool IsInvulnerable() const { return InvulnerableTime > 0.f; }
	void ClearInvulnerable() { InvulnerableTime = 0.f; }

	bool IsDead() const { return bDead; }

	/** Vida llena y vivo otra vez (al reaparecer). */
	void ResetHealth();

	/** Recuperación y temporizadores. Lo llama TickComponent con el tiempo real. */
	void Advance(float RealDeltaTime);

	float GetFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	/** > 0 justo después de recibir daño (la barra parpadea). */
	float GetHurtFlash() const { return HurtFlash; }

	FOnTemporalDeath OnDeath;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float SinceDamage = 100.f;
	float InvulnerableTime = 0.f;
	float HurtFlash = 0.f;
	bool bDead = false;
};
