// ENTROPÍA · Ojo lento (Fase 3, la pasiva de la rama Brecha)
// Mientras apuntas (clic derecho), las balas que entran a Radius de ti se frenan. Así las ves venir, las esquivas o
// las atrapas con Detener. Gasta energía solo mientras está frenando alguna.
// Pide la velocidad al árbitro del tiempo: si una burbuja ya congeló la bala, sigue congelada.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SlowEyeComponent.generated.h"

class UTemporalAimComponent;
class UTemporalEnergyComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Slow Eye (Ojo lento)"))
class ENTROPY_API USlowEyeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USlowEyeComponent();

	/** Distancia a la que empieza a frenar las balas (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "50"))
	float Radius = 350.f;

	/** Velocidad de las balas frenadas (0.25 = 4 veces más lentas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.05", ClampMax = "1"))
	float BulletDilation = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float CostPerSecond = 6.f;

	int32 NumSlowed() const { return Slowed.Num(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ReleaseAll();

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> Aim;

	TArray<TWeakObjectPtr<AActor>> Slowed;
};
