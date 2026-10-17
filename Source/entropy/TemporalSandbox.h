// ENTROPÍA · Caja de arena temporal
// Arrástralo al nivel y dale Play. Prepara al jugador y conecta las teclas:
//   F (mantener) = Paso Inverso: Luz regresa en el tiempo (gasta energía)
//   R (mantener) = las cajas regresan en el tiempo
//   E            = Eco: un fantasma repite tus últimos segundos en bucle (máx. 3)
//   G            = borrar los ecos
//   Q            = Brecha: burbuja que congela el tiempo frente a ti
//   T            = volver a soltar las cajas
//   1 / 2 / 3    = ir a la sala de Eco / Inverso / Brecha del cuarto de pruebas
//   0            = regresar a donde empezaste (las cajas)
//   4            = ir a la sala 4 · Física (torreta y objetos rompibles)
//   Z / X        = la torreta de la sala 4 dispara una bala normal / una bala invertida
//   B            = romper el objeto rompible que tengas enfrente
//   V            = Sobremarcha: el mundo lento y tú normal (prende/apaga)
//   R / Ctrl+R   = R invierte solo lo que apuntas; Ctrl+R rebobina todo (pruebas)
//   Clic derecho = apuntar (cámara al hombro; la mira selecciona)
//   Clic izq.    = Detener lo que tienes en la mira (4 s; otro clic lo suelta)
//   Shift        = dash (también en el aire, uno por salto)
// Modo tour: con -EntropiaTour en la línea de comandos recorre las salas solo, usa los poderes,
// registra los FPS (ENTROPIA_TOUR en el log), toma capturas en Saved/Tour/ y cierra el juego.
// También crea el cuarto de pruebas (ATemporalTestChamber) y la sala de física (ATemporalPhysicsLab) flotando sobre el Sandbox.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalSandbox.generated.h"

class UTemporalRecorderComponent;
class UInverseStepComponent;
class UTemporalEnergyComponent;
class UTemporalHUDWidget;
class UObjectInversionComponent;
class UOverdriveComponent;
class UTemporalAimComponent;
class UTimeStopComponent;
class UTemporalDashComponent;

/** Un paso del modo tour: qué hacer y cuánto esperar (segundos reales). */
struct FEntropiaTourStep
{
	FString Name;
	TFunction<void()> Action;
	float Seconds = 2.f;
};
class AEchoGhost;
class ATemporalTestChamber;
class ATemporalPhysicsLab;

UCLASS(meta = (DisplayName = "Temporal Sandbox"))
class ENTROPY_API ATemporalSandbox : public AActor
{
	GENERATED_BODY()

public:
	ATemporalSandbox();

	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "0", ClampMax = "40"))
	int32 NumCubes = 8;

	/** Altura desde la que caen las cajas (cm sobre el actor). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float SpawnHeight = 500.f;

	/** Segundos que recuerda el jugador (los usa el Eco y el Paso Inverso). */
	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "1"))
	float EchoSeconds = 8.f;

	/** Ecos simultáneos. Al crear uno más, desaparece el más viejo. */
	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxEchoes = 3;

	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "50"))
	float BubbleRadius = 400.f;

	/** 0 = congela, 0.2 = cámara lenta, 3 = acelera. */
	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "0"))
	float BubbleDilation = 0.f;

	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "1"))
	float BubbleSeconds = 5.f;

	/** Energía que cuesta cada burbuja (Q) y cada eco (E). */
	UPROPERTY(EditAnywhere, Category = "Entropia|Energia", meta = (ClampMin = "0"))
	float BubbleCost = 30.f;

	UPROPERTY(EditAnywhere, Category = "Entropia|Energia", meta = (ClampMin = "0"))
	float EchoCost = 10.f;

	/** Crea el cuarto de pruebas con las 3 salas (Eco, Inverso, Brecha). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnTestChamber = true;

	/** Dónde aparece el cuarto respecto al Sandbox (cm). Alto para no chocar con el nivel. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector ChamberOffset = FVector(0.f, 0.f, 3000.f);

	/** Crea la sala 4 · Física (torreta, caja y muro rompibles). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnPhysicsLab = true;

	/** Dónde aparece la sala de física respecto al Sandbox (cm). A un lado del cuarto de pruebas. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector PhysicsLabOffset = FVector(0.f, 3500.f, 3000.f);

	/** Empezar el Play dentro de la sala 1. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bStartInChamber = true;

	/** Crea una placa con puerta suelta junto al Sandbox (la versión vieja del puzzle del Eco). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	bool bSpawnPlateDemo = false;

	/** Dónde aparece la placa respecto al Sandbox (cm). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector PlateOffset = FVector(0.f, -600.f, 60.f);

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void Setup();
	void SpawnCubes();
	void ShowHelp();

	void OnInversePressed();
	void OnInverseReleased();
	void OnRewindPressed();
	void OnRewindReleased();
	void OnEcho();
	void OnClearEchoes();
	void OnBubble();
	void OnRespawnCubes();
	void OnRoom1();
	void OnRoom2();
	void OnRoom3();
	void GoToRoom(int32 RoomIndex);
	void OnBackToStart();
	void OnRoom4();
	void OnFire();
	void OnFireInverted();
	void OnBreakInFront();
	void OnOverdrive();
	void OnAimPressed();
	void OnAimReleased();
	void OnPrimaryPressed();
	void OnDash();
	/** Modo tour (pruebas automáticas en juego). */
	void StartTour();
	void BuildChaosSteps();
	void BeginTourStep();
	void FinishTourStep();
	void TourRewindAll(bool bStart);

	/** Antes de teletransportar al jugador: soltar F y R. */
	void EndPlayerPowersForTeleport();
	TArray<AActor*> GatherRewindables() const;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Cubes;

	UPROPERTY()
	TArray<TObjectPtr<AEchoGhost>> Echoes;

	UPROPERTY()
	TObjectPtr<UTemporalRecorderComponent> PlayerRecorder;

	UPROPERTY()
	TObjectPtr<UInverseStepComponent> PlayerInverseStep;

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> PlayerEnergy;

	UPROPERTY()
	TObjectPtr<UTemporalHUDWidget> EnergyHUD;

	UPROPERTY()
	TObjectPtr<UObjectInversionComponent> PlayerInversion;

	UPROPERTY()
	TObjectPtr<UOverdriveComponent> PlayerOverdrive;

	UPROPERTY()
	TObjectPtr<UTemporalAimComponent> PlayerAim;

	UPROPERTY()
	TObjectPtr<UTimeStopComponent> PlayerTimeStop;

	UPROPERTY()
	TObjectPtr<UTemporalDashComponent> PlayerDash;

	/** La R actual se pulsó con Ctrl (rebobinar todo) y no como Invertir objeto. */
	bool bRewindingAll = false;

	UPROPERTY()
	TObjectPtr<ATemporalTestChamber> Chamber;

	UPROPERTY()
	TObjectPtr<ATemporalPhysicsLab> PhysicsLab;

	FVector PlayerStartLocation = FVector::ZeroVector;

	TArray<FEntropiaTourStep> TourSteps;
	int32 TourIndex = INDEX_NONE;
	double TourStepStart = 0.0;
	bool bTourShotTaken = false;
	int32 TourFrames = 0;
	double TourFrameTime = 0.0;
	float TourMinFps = 0.f;
	int32 TourSkipMinFrames = 0;
};
