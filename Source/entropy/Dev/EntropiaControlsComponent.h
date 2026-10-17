// ENTROPÍA · Teclas del banco de pruebas (Dev)
// Conecta cada tecla con su poder o su sala (BindKey, sin Enhanced Input todavía) y muestra la ayuda en pantalla.
//   Clic der. apuntar · Clic izq. Detener o rematar · Shift tocar = dash, mantener = correr · C agacharse · F Paso Inverso · R invertir · Ctrl+R todo ·
//   V Sobremarcha · E agarrar · H eco · G borrar ecos · Q burbuja · T soltar cajas · 1-5 salas · 0 inicio · Z/X/B sala 4

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaControlsComponent.generated.h"

class UEntropiaPlayerPowers;
class UEntropiaRoomsComponent;
class UInputComponent;

UCLASS(ClassGroup = (Entropia), meta = (DisplayName = "Entropia Controls"))
class ENTROPY_API UEntropiaControlsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaControlsComponent();

	/** Conecta las teclas en el InputComponent del Sandbox. */
	void BindKeys(UInputComponent* Input, UEntropiaPlayerPowers* InPowers, UEntropiaRoomsComponent* InRooms);

	void ShowHelp() const;

private:
	void OnInversePressed();
	void OnInverseReleased();
	void OnRewindPressed();
	void OnRewindReleased();
	void OnEcho();
	void OnClearEchoes();
	void OnBubble();
	void OnRespawnCubes();
	void OnRoom0();
	void OnRoom1();
	void OnRoom2();
	void OnRoom3();
	void OnRoom4();
	void OnRoom5();
	void OnRoom6();
	void OnRoom7();
	void OnFire();
	void OnFireInverted();
	void OnBreakInFront();
	void OnOverdrive();
	void OnAimPressed();
	void OnAimReleased();
	void OnPrimaryPressed();
	void OnPrimaryReleased();
	void OnNextWeapon();
	void OnPreviousWeapon();
	void OnDash();
	void OnSprintReleased();
	void OnCrouch();
	void OnGrab();
	void OnTimeStopLevel();

	UPROPERTY()
	TObjectPtr<UEntropiaPlayerPowers> Powers;

	UPROPERTY()
	TObjectPtr<UEntropiaRoomsComponent> Rooms;

	/** La R actual se pulsó con Ctrl (rebobinar todo) y no como Invertir objeto. */
	bool bRewindingAll = false;
};
