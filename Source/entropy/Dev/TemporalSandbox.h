// ENTROPÍA · Caja de arena temporal
// Arrástralo al nivel y dale Play. Al empezar:
//   1) equipa al jugador con todos los poderes y el HUD   → UEntropiaPlayerPowers (EntropiaPlayerPowers.h)
//   2) crea las cajas y las salas 1 a 5                     → UEntropiaRoomsComponent
//   3) conecta las teclas y muestra la ayuda                → UEntropiaControlsComponent
//   4) con -EntropiaTour, el juego se prueba solo           → UEntropiaTourComponent
// Si el jugador muere, al siguiente frame suelta todo y reaparece en la sala 5.
// Las opciones (cuántas cajas, dónde aparece cada sala, costo de la burbuja…) están en sus componentes, en Details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalSandbox.generated.h"

class UEntropiaControlsComponent;
class UEntropiaPlayerPowers;
class UEntropiaRoomsComponent;
class UEntropiaTourComponent;

UCLASS(meta = (DisplayName = "Temporal Sandbox"))
class ENTROPY_API ATemporalSandbox : public AActor
{
	GENERATED_BODY()

public:
	ATemporalSandbox();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UEntropiaPlayerPowers> Powers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UEntropiaRoomsComponent> Rooms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UEntropiaControlsComponent> Controls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UEntropiaTourComponent> Tour;

protected:
	virtual void BeginPlay() override;

private:
	void Setup();
	void OnPlayerDied();
	void RespawnAfterDeath();
};
