// ENTROPÍA · Energía compartida (Fase 1)
// Una sola barra para todos los poderes: F, R y V la gastan de forma continua; Q y E de golpe.
// Se mide en TIEMPO REAL (FApp::GetDeltaTime), así la Sobremarcha y las burbujas no cambian su ritmo.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalEnergyComponent.generated.h"

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Energy"))
class ENTROPY_API UTemporalEnergyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalEnergyComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1"))
	float MaxEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float RegenPerSecond = 20.f;

	/** Segundos sin gastar antes de que empiece a recargarse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float RegenDelay = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	float Energy = 100.f;

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool CanAfford(float Amount) const { return Energy >= Amount; }

	/** Gasto de golpe. Si no alcanza, no gasta nada y devuelve false. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool Spend(float Amount);

	/** Gasto continuo. Devuelve false cuando la energía llega a 0. */
	bool Drain(float PerSecond, float RealDeltaTime);

	/** Aviso visual de "no alcanza" (la barra parpadea en rojo). */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Deny() { DenyFlash = 0.3f; }

	/** Recarga y temporizadores. Lo llama TickComponent con el tiempo real. */
	void Advance(float RealDeltaTime);

	UFUNCTION(BlueprintPure, Category = "Entropia")
	float GetFraction() const { return MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f; }

	float GetDenyFlash() const { return DenyFlash; }

	/** Qué poder está gastando ahora (colorea la barra). NAME_None = reposo. */
	void SetActivePower(FName Power) { ActivePower = Power; }
	FName GetActivePower() const { return ActivePower; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void MarkUsed() { TimeSinceUse = 0.f; }

	float TimeSinceUse = 0.f;
	float DenyFlash = 0.f;
	FName ActivePower = NAME_None;
};
