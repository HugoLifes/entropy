// ENTROPÍA · Sala 4 · Física temporal (paso 5)
// Un cuarto construido en runtime con cubos del motor (sin assets), al lado del cuarto de pruebas.
// Torreta al fondo apuntando a: una caja rompible → un muro rompible → la pared de tiro.
//   Z = disparar · X = bala invertida · R = rebobinar (la bala vuelve al cañón y todo se rearma) · B = romper lo de enfrente

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalPhysicsLab.generated.h"

class ATemporalBreakable;
class ATemporalTurret;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalPhysicsLab : public AActor
{
	GENERATED_BODY()

public:
	ATemporalPhysicsLab();

	/** Lleva al jugador a la entrada de la sala y la deja como nueva. */
	void TeleportPlayerHere();

	/** Rearma todo y borra las balas. */
	void ResetLab();

	ATemporalTurret* GetTurret() const { return Turret; }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision = true);
	ATemporalBreakable* AddBreakable(const FVector& Center, const FVector& Size, const FIntVector& Pieces, const FLinearColor& Color);
	void ShowHint() const;

	UPROPERTY()
	TObjectPtr<ATemporalTurret> Turret;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalBreakable>> Breakables;

	bool bPlayerInside = false;
};
