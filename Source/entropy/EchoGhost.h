// ENTROPÍA · Eco
// Un actor fantasma que repite hacia adelante una grabación del Registro Temporal.
// Tiene cápsula (puede pisar placas de presión) y, si la grabación trae pose,
// una "marioneta" (PoseableMesh) que repite la animación exacta del jugador.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalRecorderComponent.h"
#include "EchoGhost.generated.h"

class UCapsuleComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEchoFinished);

UCLASS()
class ENTROPY_API AEchoGhost : public AActor
{
	GENERATED_BODY()

public:
	AEchoGhost();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UPoseableMeshComponent> PoseMesh;

	/** Copia la malla, su posición relativa y el tamaño de cápsula de un personaje. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void SetupFromCharacterMesh(USkeletalMeshComponent* SourceMesh, float CapsuleRadius, float CapsuleHalfHeight);

	/** Empieza a repetir la grabación desde su primer frame. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void StartReplay(const TArray<FTemporalFrame>& InFrames, bool bLoop);

	/** Si no está en bucle, el eco se destruye solo al terminar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bDestroyWhenFinished = true;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	bool bPlaying = false;

	UPROPERTY(BlueprintAssignable, Category = "Entropia")
	FOnEchoFinished OnEchoFinished;

	virtual void Tick(float DeltaTime) override;

private:
	void ApplyFrame(const FTemporalFrame& Frame);

	UPROPERTY()
	TArray<FTemporalFrame> Frames;

	int32 FrameIndex = 0;
	float Accumulator = 0.f;
	bool bLooping = false;
};
