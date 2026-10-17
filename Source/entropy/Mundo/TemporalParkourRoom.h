// ENTROPÍA · Sala 6 · Parkour (Fase 3b)
// Un cuarto construido en runtime con cubos (sin assets) para practicar el parkour con Espacio:
//   carril izquierdo: barda baja (dash vault) → mesa larga (el gato, corriendo) → plataforma baja (subir)
//   carril central:   muro de 2 m (trepar) → muro de 3 m (correr por la pared, corriendo)
//   carril derecho:   cornisas para colgarse y un balcón alto para practicar caídas

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalParkourRoom.generated.h"

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalParkourRoom : public AActor
{
	GENERATED_BODY()

public:
	ATemporalParkourRoom();

	void TeleportPlayerHere();

	/** Posición del mundo a partir de una local de la sala (para el modo tour). */
	FVector RoomToWorld(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }

	/** Pone al jugador en un punto local mirando hacia +X de la sala (modo tour). */
	void PlacePlayer(const FVector& Local) const;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void ShowHint() const;

	bool bPlayerInside = false;
};
