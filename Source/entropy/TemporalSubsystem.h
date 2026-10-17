// ENTROPÍA · Lista de todo lo que tiene Registro Temporal en el mundo (Fase 1)
// Cada UTemporalRecorderComponent se anota al empezar y se borra al terminar.
// La usan "Invertir objeto" (R apuntado) y "rebobinar todo" (Shift+R), sin recorrer todos los actores.

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

private:
	TArray<TWeakObjectPtr<UTemporalRecorderComponent>> Recorders;
};
