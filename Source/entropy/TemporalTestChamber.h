// ENTROPÍA · Cuarto de pruebas (paso 3)
// Construye solo, en tiempo de ejecución, 3 salas seguidas sobre el eje X (no hay que modelar nada):
//   Sala 1 · ECO     : placa + puerta. Graba un eco sobre la placa y pasa mientras él la pisa.
//   Sala 2 · INVERSO : plataformas que se caen. Rebobínalas (R) y quedan ANCLADAS. Si caes, F te rescata.
//   Sala 3 · BRECHA  : prensa que aplasta. Congélala arriba con la burbuja (Q) y pasa por debajo.
// La crea el Temporal Sandbox (bSpawnTestChamber). Teclas 1/2/3 = ir a cada sala.
// Coordenadas locales: piso en Z = 0, la sala avanza en +X, de Y = -925 a 925.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalTestChamber.generated.h"

class ACrumblePlatform;
class AMovingHazard;
class APressurePlateDoor;

UCLASS(meta = (DisplayName = "Temporal Test Chamber"))
class ENTROPY_API ATemporalTestChamber : public AActor
{
	GENERATED_BODY()

public:
	ATemporalTestChamber();

	/** Lleva al jugador al inicio de una sala (0, 1 o 2) y reinicia esa sala. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void TeleportToRoom(int32 RoomIndex);

	/** Plataformas de la sala 2 (el Sandbox las rebobina con R). */
	const TArray<TObjectPtr<ACrumblePlatform>>& GetPlatforms() const { return Platforms; }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision = true);
	void ResetFallenPlatforms(bool bAlsoAnchored);
	void ShowRoomHint(int32 RoomIndex);
	int32 RoomAt(float LocalX) const;
	FVector RoomStart(int32 RoomIndex) const;

	UPROPERTY()
	TArray<TObjectPtr<ACrumblePlatform>> Platforms;

	UPROPERTY()
	TObjectPtr<AMovingHazard> Hazard;

	UPROPERTY()
	TObjectPtr<APressurePlateDoor> PlateDoor;

	int32 CurrentRoom = INDEX_NONE;
	bool bGoalReached = false;
};
