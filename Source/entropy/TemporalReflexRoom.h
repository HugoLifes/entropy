// ENTROPÍA · Sala 5 · Reflejos (Fase 3)
// Un cuarto construido en runtime con cubos del motor (sin assets), al otro lado del cuarto de pruebas.
// Al fondo, 3 torretas que te siguen y te disparan si te ven; columnas para cubrirte; maniquíes para los remates.
// Aquí se prueban el dash, la esquiva perfecta (tiempo de reflejo), el Ojo lento, Detener y los remates.
// Si mueres, reapareces en la entrada con la vida llena.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalReflexRoom.generated.h"

class ATemporalTurret;
class ATemporalMannequin;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalReflexRoom : public AActor
{
	GENERATED_BODY()

public:
	ATemporalReflexRoom();

	/** Lleva al jugador a la entrada (vida llena) y deja la sala como nueva. */
	void TeleportPlayerHere();

	/** Torretas a su lugar y sin balas. */
	void ResetRoom();

	/** Después de morir: a la entrada, vida llena y un momento de protección. */
	void RespawnPlayer();

	const TArray<TObjectPtr<ATemporalTurret>>& GetTurrets() const { return Turrets; }

	/** 0 = de lado en la zona segura (para practicar) · 1 = mira a los lados · 2 = te mira de frente (hay que congelarlo). */
	const TArray<TObjectPtr<ATemporalMannequin>>& GetMannequins() const { return Mannequins; }

	/** Posición del mundo a partir de una local de la sala (para el modo tour). */
	FVector RoomToWorld(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision = true);
	void PlacePlayerAtStart();
	void ShowHint() const;
	/** ¿El jugador pasó la franja roja? (la entrada es zona segura) */
	bool IsPlayerInFireZone() const;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalTurret>> Turrets;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalMannequin>> Mannequins;

	TArray<FRotator> TurretStartRotations;
	bool bPlayerInside = false;
};
