// ENTROPÍA · Salas del banco de pruebas (Dev)
// Crea las cajas con física, el cuarto de pruebas (salas 1-3), la sala 4 · Física y la sala 5 · Reflejos, y lleva al
// jugador de una a otra. También las acciones de la sala 4 (disparar, bala invertida, romper) y "rebobinar todo".
// Vive en el Temporal Sandbox; las salas aparecen respecto a él.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaRoomsComponent.generated.h"

class ATemporalPhysicsLab;
class ATemporalReflexRoom;
class ATemporalTestChamber;
class UEntropiaPlayerPowers;

UCLASS(ClassGroup = (Entropia), meta = (DisplayName = "Entropia Rooms"))
class ENTROPY_API UEntropiaRoomsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaRoomsComponent();

	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "0", ClampMax = "40"))
	int32 NumCubes = 8;

	/** Altura desde la que caen las cajas (cm sobre el Sandbox). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float SpawnHeight = 500.f;

	/** Crea el cuarto de pruebas con las 3 salas (Eco, Inverso, Brecha). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnTestChamber = true;

	/** Dónde aparece el cuarto respecto al Sandbox (cm). Alto para no chocar con el nivel. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector ChamberOffset = FVector(0.f, 0.f, 3000.f);

	/** Crea la sala 4 · Física (torreta, caja y muro rompibles). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnPhysicsLab = true;

	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector PhysicsLabOffset = FVector(0.f, 3500.f, 3000.f);

	/** Crea la sala 5 · Reflejos (torretas que te siguen, columnas, maniquíes). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnReflexRoom = true;

	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector ReflexRoomOffset = FVector(0.f, -3500.f, 3000.f);

	/** Empezar el Play dentro de la sala 1. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bStartInChamber = true;

	/** Crea una placa con puerta suelta junto al Sandbox (la versión vieja del puzzle del Eco). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnPlateDemo = false;

	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector PlateOffset = FVector(0.f, -600.f, 60.f);

	/** Crea todo. Recuerda dónde empezó el jugador (tecla 0). */
	void BuildAll(UEntropiaPlayerPowers* InPowers);

	/** 0 = inicio (las cajas) · 1, 2, 3 = salas del cuarto de pruebas · 4 = Física · 5 = Reflejos. */
	void GoToRoom(int32 Key);

	/** Después de morir: a la entrada de la sala 5 (o al inicio si no hay). */
	void RespawnPlayer();

	/** T: vuelve a soltar las cajas. */
	void SpawnCubes();
	/** Sala 4: disparar, bala invertida, romper lo de enfrente. */
	void Fire();
	void FireInverted();
	void BreakInFront();

	/** Ctrl+R (pruebas): todo lo que tiene Registro, menos el jugador, regresa en el tiempo o deja de hacerlo. */
	void RewindAll(bool bStart);
	TArray<AActor*> GatherRewindables() const;

	UPROPERTY() TArray<TObjectPtr<AActor>> Cubes;
	UPROPERTY() TObjectPtr<ATemporalTestChamber> Chamber;
	UPROPERTY() TObjectPtr<ATemporalPhysicsLab> PhysicsLab;
	UPROPERTY() TObjectPtr<ATemporalReflexRoom> ReflexRoom;

private:
	void BackToStart();
	FTransform PlaceAt(const FVector& Offset) const;

	UPROPERTY()
	TObjectPtr<UEntropiaPlayerPowers> Powers;

	FVector PlayerStartLocation = FVector::ZeroVector;
};
