// ENTROPÍA · Sala 8 · Tiroteo (Fase 4)
// Una arena para lucir los disparos con el tiempo: 4 dummies armados detrás de muros a 12, 18, 25 y 30 m, un muro rompible
// frente a uno de ellos, cajas rompibles en medio y tus coberturas cerca de la entrada. La pista en pantalla enseña las
// jugadas: Lluvia detenida, Devuélvesela y Recarga inversa (Último en caer se descubre solo).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalShootingRoom.generated.h"

class ATemporalArmedDummy;
class ATemporalBreakable;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalShootingRoom : public AActor
{
	GENERATED_BODY()

public:
	ATemporalShootingRoom();

	/** Lleva al jugador a la entrada (vida llena) y deja la sala como nueva. */
	void TeleportPlayerHere();
	void ResetRoom();

	const TArray<TObjectPtr<ATemporalArmedDummy>>& GetDummies() const { return Dummies; }
	/** El muro rompible (las cajas no cuentan). */
	const TArray<TObjectPtr<ATemporalBreakable>>& GetBreakables() const { return Breakables; }

	FVector RoomToWorld(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	void PlacePlayer(const FVector& Local, float YawOffset = 0.f) const;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void ShowHint() const;
	ATemporalBreakable* AddBreakable(const FVector& Center, const FVector& Size, const FIntVector& Pieces, const FLinearColor& Color);

	UPROPERTY()
	TArray<TObjectPtr<ATemporalArmedDummy>> Dummies;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalBreakable>> Breakables;

	UPROPERTY()
	TArray<TObjectPtr<ATemporalBreakable>> Crates;

	bool bPlayerInside = false;
};
