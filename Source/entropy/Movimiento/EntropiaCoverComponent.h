// ENTROPÍA · Cobertura (Fase 3b)
// C junto a una pared o una barda te pega a ella: en una barda baja quedas agachado; en una pared alta, de pie.
// Te mueves a lo largo (sin pasarte de la orilla). Al apuntar te asomas: en la baja te levantas por encima; en la alta,
// si estás en una esquina, sales de lado. Sales con C, empujando el stick hacia atrás, con un salto o un dash, o si la
// cobertura desaparece (se rompe).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaCoverComponent.generated.h"

class ACharacter;

enum class EEntropiaCover : uint8
{
	None,
	Low,
	High
};

namespace EntropiaCover
{
	constexpr float LowMinHeight = 70.f;
	constexpr float HighMinHeight = 160.f;
	/** Qué tan lejos puede estar la pared al presionar C. */
	constexpr float MaxDistance = 120.f;

	/** Qué tipo de cobertura da algo de esta altura (sobre los pies). Lógica pura (probada). */
	ENTROPY_API EEntropiaCover KindForHeight(float Height);
}

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Cover"))
class ENTROPY_API UEntropiaCoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaCoverComponent();

	/** Cuánto sales de lado al asomarte desde una esquina de una pared alta (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float PeekOut = 75.f;

	/** C: si hay una pared o barda cerca, te cubres. False si no hay nada (entonces C agacha). */
	bool TryEnter();
	void Exit();

	bool IsInCover() const { return Kind != EEntropiaCover::None; }
	EEntropiaCover GetKind() const { return Kind; }
	/** -1 / +1 si estás en la esquina izquierda / derecha de la pared (vista desde ti); 0 si no. */
	int32 GetCornerSide() const { return CornerSide; }
	bool IsPeeking() const { return bPeeking; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	ACharacter* GetCharacter() const;
	/** ¿Hay pared detrás (hacia la cobertura) en este punto? */
	bool WallAt(const FVector& Location, float* OutHeight = nullptr) const;
	void UpdatePeek(bool bAiming);

	EEntropiaCover Kind = EEntropiaCover::None;
	/** Normal de la pared (apunta hacia ti). */
	FVector CoverNormal = FVector::ForwardVector;
	FVector LastValidLocation = FVector::ZeroVector;
	FVector PeekBase = FVector::ZeroVector;
	int32 CornerSide = 0;
	bool bPeeking = false;
	float PushAwayTime = 0.f;
};
