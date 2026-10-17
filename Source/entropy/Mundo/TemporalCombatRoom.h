// ENTROPÍA · Sala 7 · Combate (Fase 3b)
// Un cuarto construido en runtime: al fondo, tres dummies armados cubiertos detrás de muros que se asoman y te
// disparan; adelante, bardas bajas y una columna para que tú te cubras. Para practicar cobertura, Detener, golpes,
// agarrar, desarmar y romper el cuello.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalCombatRoom.generated.h"

class ATemporalArmedDummy;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalCombatRoom : public AActor
{
	GENERATED_BODY()

public:
	ATemporalCombatRoom();

	/** Lleva al jugador a la entrada (vida llena) y deja la sala como nueva. */
	void TeleportPlayerHere();
	void ResetRoom();

	const TArray<TObjectPtr<ATemporalArmedDummy>>& GetDummies() const { return Dummies; }

	FVector RoomToWorld(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	void PlacePlayer(const FVector& Local, float YawOffset = 0.f) const;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void ShowHint() const;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalArmedDummy>> Dummies;

	bool bPlayerInside = false;
};
