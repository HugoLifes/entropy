// ENTROPÍA · Lista de todo lo que tiene Registro Temporal en el mundo (Fase 1)
// Cada UTemporalRecorderComponent se anota al empezar y se borra al terminar.
// La usan la mira (UTemporalAimComponent) y "rebobinar todo" (Ctrl+R), sin recorrer todos los actores.
// Fase 3: también guarda los "objetivos de tiempo" sin Registro (maniquíes, torretas) que la mira puede seleccionar.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TemporalSubsystem.generated.h"

class UTemporalRecorderComponent;

UCLASS()
class ENTROPY_API UTemporalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(UTemporalRecorderComponent* Recorder);
	void Unregister(UTemporalRecorderComponent* Recorder);

	/** Solo los que siguen vivos. */
	TArray<UTemporalRecorderComponent*> GetRecorders() const;

	/** Actores sin Registro que la mira también puede seleccionar (para Detener o rematar). */
	void RegisterTimeTarget(AActor* Actor);
	void UnregisterTimeTarget(AActor* Actor);
	TArray<AActor*> GetTimeTargets() const;
	bool IsTimeTarget(const AActor* Actor) const;

private:
	TArray<TWeakObjectPtr<UTemporalRecorderComponent>> Recorders;
	TArray<TWeakObjectPtr<AActor>> TimeTargets;
};
