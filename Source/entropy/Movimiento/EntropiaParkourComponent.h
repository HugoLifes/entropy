// ENTROPÍA · Parkour (Fase 3b)
// Espacio hace la acción según lo que tengas enfrente: dash vault (barda baja), el gato (kong, corriendo sobre algo
// más largo), subir, trepar o correr por la pared hacia arriba. Si no hay nada, es un salto normal.
// Mide el obstáculo con trazos (altura, fondo, espacio arriba y del otro lado), decide con EntropiaParkour::Classify
// y mueve al personaje por EntropiaParkour::PathPoint. La animación (DA_Animaciones) solo es el aspecto.
// Mientras dura, el CharacterMovement queda apagado (MOVE_None); siempre se puede cancelar (F, morir, teletransporte).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaParkour.h"
#include "EntropiaParkourComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Parkour"))
class ENTROPY_API UEntropiaParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaParkourComponent();

	/** Velocidad con la que sales de un dash vault y del gato (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float VaultExitSpeed = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float KongExitSpeed = 650.f;

	/** Espacio: si hay algo que saltar o subir enfrente, lo hace y devuelve true. Si no, false (salto normal). */
	bool TryStart();

	/**
	 * Mide lo que hay enfrente (lo usa TryStart y el modo tour). OutTop = dónde quedarías de pie arriba;
	 * OutLanding = dónde caerías del otro lado (si hay piso).
	 */
	FEntropiaObstacle Measure(FVector& OutEdge, FVector& OutTop, FVector& OutLanding, FVector& OutInto) const;

	bool IsBusy() const { return Current != EEntropiaTraversal::None; }
	EEntropiaTraversal GetCurrent() const { return Current; }
	EEntropiaTraversal GetLast() const { return Last; }

	/** Corta lo que esté haciendo y deja caer al personaje. */
	void Cancel();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	ACharacter* GetCharacter() const;
	void Finish();
	/** Hacia dónde mira el intento: el stick si lo mueves; si no, hacia donde miras. */
	FVector IntentDirection() const;

	EEntropiaTraversal Current = EEntropiaTraversal::None;
	EEntropiaTraversal Last = EEntropiaTraversal::None;
	float Elapsed = 0.f;
	float Duration = 0.f;
	FVector PathStart = FVector::ZeroVector;
	FVector PathEdge = FVector::ZeroVector;
	FVector PathEnd = FVector::ZeroVector;
	FVector ExitDirection = FVector::ForwardVector;
};
