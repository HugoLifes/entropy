// ENTROPÍA · Detener (Fase 3, pedido por Hugo: "seleccionar objetos que podamos detener, como las balas")
// Clic izquierdo: lo que tienes en la mira se queda congelado en el tiempo dentro de una burbujita propia
// (una ATimeBubble con un solo objetivo). Cuesta energía; máximo 3 cosas a la vez (la cuarta suelta la más vieja).
// Clic sobre algo que tú detuviste lo suelta antes. Si lo inviertes con R, sale de su burbujita y regresa al pasado.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeStopComponent.generated.h"

class ATimeBubble;
class UTemporalAimComponent;
class UTemporalEnergyComponent;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Time Stop (Detener)"))
class ENTROPY_API UTimeStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTimeStopComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float Cost = 15.f;

	/** Segundos reales que dura detenido. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float StopSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxStopped = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor ShellColor = FLinearColor(0.75f, 0.92f, 1.f, 0.4f);

	/** Detiene lo que está en la mira (o lo suelta, si ya lo habías detenido). False si no hay objetivo o energía. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool StopAimed();

	/** Detiene un actor dado (sin apuntar). Lo usa el modo tour. */
	bool StopActor(AActor* Target);

	void ReleaseActor(AActor* Target);
	void ReleaseAll();

	bool IsStoppedByMe(const AActor* Target) const;
	int32 NumStopped() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ATimeBubble* FindShell(const AActor* Target) const;
	void ForgetFinishedShells();

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> Aim;

	TArray<TWeakObjectPtr<ATimeBubble>> Shells;
};
