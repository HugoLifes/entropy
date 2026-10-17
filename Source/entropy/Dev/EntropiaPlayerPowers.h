// ENTROPÍA · Poderes del jugador (Dev)
// Equipa al jugador con todos los poderes y el HUD, y hace las acciones que usan varios poderes a la vez:
// crear ecos, lanzar la burbuja, el clic izquierdo (rematar o Detener) y soltar todo al teletransportarse o al morir.
// Vive en el Temporal Sandbox. Los números de cada poder están en su propio componente.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaPlayerPowers.generated.h"

class AEchoGhost;
class UInverseStepComponent;
class UObjectInversionComponent;
class UOverdriveComponent;
class USlowEyeComponent;
class UTakedownComponent;
class UTemporalAimComponent;
class UTemporalDashComponent;
class UEntropiaLocomotionComponent;
class UEntropiaParkourComponent;
class UTemporalEnergyComponent;
class UTemporalHealthComponent;
class UTemporalHUDWidget;
class UTemporalRecorderComponent;
class UTimeStopComponent;

UCLASS(ClassGroup = (Entropia), meta = (DisplayName = "Entropia Player Powers"))
class ENTROPY_API UEntropiaPlayerPowers : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaPlayerPowers();

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

	/** Agrega al jugador todos los poderes (o usa los que ya tenga) y pone el HUD en pantalla. */
	void EquipPlayer(APawn* Pawn);

	/** E: un fantasma repite tus últimos segundos en bucle. */
	void CreateEcho();
	/** G: borra los ecos. */
	void ClearEchoes();
	/** Q: burbuja frente a ti. */
	void SpawnBubble();
	/** Clic izquierdo: remata si el HUD lo muestra; si no, detiene lo que está en la mira. */
	void PrimaryAction();
	void TryDash();
	void ToggleOverdrive();

	/** Antes de teletransportar al jugador: soltar F y R. */
	void EndForTeleport();
	/** Al morir: soltar TODO (F, R, Sobremarcha, apuntar, lo detenido, dash, reflejo, remate y ecos). */
	void CancelAll();

	int32 NumEchoes() const;

	UPROPERTY() TObjectPtr<UTemporalRecorderComponent> Recorder;
	UPROPERTY() TObjectPtr<UTemporalEnergyComponent> Energy;
	UPROPERTY() TObjectPtr<UTemporalHealthComponent> Health;
	UPROPERTY() TObjectPtr<UInverseStepComponent> InverseStep;
	UPROPERTY() TObjectPtr<UTemporalAimComponent> Aim;
	UPROPERTY() TObjectPtr<UObjectInversionComponent> Inversion;
	UPROPERTY() TObjectPtr<UTimeStopComponent> TimeStop;
	UPROPERTY() TObjectPtr<UTemporalDashComponent> Dash;
	UPROPERTY() TObjectPtr<UEntropiaLocomotionComponent> Locomotion;
	UPROPERTY() TObjectPtr<UEntropiaParkourComponent> Parkour;
	UPROPERTY() TObjectPtr<USlowEyeComponent> SlowEye;
	UPROPERTY() TObjectPtr<UTakedownComponent> Takedown;
	UPROPERTY() TObjectPtr<UOverdriveComponent> Overdrive;
	UPROPERTY() TObjectPtr<UTemporalHUDWidget> HUD;

private:
	UPROPERTY()
	TArray<TObjectPtr<AEchoGhost>> Echoes;
};
