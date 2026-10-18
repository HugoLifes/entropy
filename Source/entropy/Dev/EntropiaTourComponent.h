// ENTROPÍA · Modo tour (Dev)
// Con -EntropiaTour en la línea de comandos, el juego se prueba solo: recorre las salas, usa los poderes, registra
// FPS, actores y Registros por paso (ENTROPIA_TOUR en el log), toma capturas en Saved/Tour/ y cierra el juego.
// -EntropiaTour=caos hace cosas raras a propósito para buscar errores y fugas.
// Los pasos están en EntropiaTourNormal.cpp y EntropiaTourCaos.cpp.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaTourComponent.generated.h"

class ATemporalProjectile;
class UEntropiaPlayerPowers;
class UEntropiaRoomsComponent;

/** Un paso del modo tour: qué hacer y cuánto esperar (segundos reales). */
struct FEntropiaTourStep
{
	FString Name;
	TFunction<void()> Action;
	float Seconds = 2.f;
};

UCLASS(ClassGroup = (Entropia), meta = (DisplayName = "Entropia Tour"))
class ENTROPY_API UEntropiaTourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaTourComponent();

	/** ¿La línea de comandos pide el tour? */
	static bool IsRequested();

	void Start(UEntropiaPlayerPowers* InPowers, UEntropiaRoomsComponent* InRooms);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void BuildNormalSteps();
	void BuildChaosSteps();
	void BeginTourStep();
	void FinishTourStep();

	/** Una bala que le pega al jugador, desde From hacia Direction (se borra con ClearTourTemps). */
	ATemporalProjectile* SpawnTourBullet(const FVector& From, const FVector& Direction);
	/** Una bala del jugador (daña maniquíes), como las del revólver o el fusil. */
	ATemporalProjectile* SpawnPlayerTourBullet(const FVector& From, const FVector& Direction, float TorsoDamage, float HeadDamage, float Impulse);
	void ClearTourTemps();
	/** Gira la cámara para que el centro de la mira quede sobre Target. */
	void LookAtPoint(const FVector& Target);

	UPROPERTY()
	TObjectPtr<UEntropiaPlayerPowers> Powers;

	UPROPERTY()
	TObjectPtr<UEntropiaRoomsComponent> Rooms;

	TArray<FEntropiaTourStep> TourSteps;
	int32 TourIndex = INDEX_NONE;
	double TourStepStart = 0.0;
	bool bTourShotTaken = false;
	int32 TourFrames = 0;
	double TourFrameTime = 0.0;
	float TourMinFps = 0.f;
	int32 TourSkipMinFrames = 0;

	/** Cosas que creó un paso del tour y hay que borrar después. */
	TArray<TWeakObjectPtr<AActor>> TourTemps;

	/** Una posición que un paso guarda para que el siguiente mida cuánto se movió algo. */
	FVector TourMark = FVector::ZeroVector;
};
