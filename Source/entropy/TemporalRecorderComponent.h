// ENTROPÍA · Registro Temporal
// Graba la transformación (y la velocidad física) de su actor cada frame
// y puede reproducirla hacia atrás. Es la base de los tres poderes:
//   Inverso = reproducir hacia atrás · Eco = reproducir hacia adelante · Brecha = cambiar la velocidad.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalRecorderComponent.generated.h"

class UPrimitiveComponent;

/** Un instante grabado. */
USTRUCT(BlueprintType)
struct FTemporalFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	FTransform Transform;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	FVector LinearVelocity = FVector::ZeroVector;

	/** En grados por segundo. */
	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	FVector AngularVelocity = FVector::ZeroVector;

	/** Cuánto duró este frame (para reproducir al mismo ritmo). */
	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	float DeltaTime = 0.f;

	/** ¿Iba en el aire? (solo personajes) */
	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	bool bFalling = false;

	/** Pose del esqueleto en espacio de hueso (solo si bRecordPose). Vacío si no se graba pose. */
	UPROPERTY()
	TArray<FTransform> Pose;
};

UENUM(BlueprintType)
enum class ETemporalState : uint8
{
	Recording,
	Rewinding,
	Paused
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTemporalRewindFinished);

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Recorder"))
class ENTROPY_API UTemporalRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalRecorderComponent();

	/** Cuántos segundos hacia atrás se recuerdan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float MaxRecordSeconds = 10.f;

	/** 1 = rebobina a velocidad real, 2 = el doble de rápido. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.1"))
	float RewindSpeed = 1.f;

	/** Al soltar el rebobinado, el objeto sale con la inercia invertida (estilo "objeto invertido"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bInvertMomentumOnRelease = false;

	/** Graba también la pose del esqueleto (para que el Eco y el Paso Inverso se animen). Úsalo solo en personajes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bRecordPose = false;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	ETemporalState State = ETemporalState::Recording;

	/** Se dispara al terminar de rebobinar (soltaste la tecla o se acabó la grabación). */
	UPROPERTY(BlueprintAssignable, Category = "Entropia")
	FOnTemporalRewindFinished OnRewindFinished;

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void StartRewind();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void StopRewind();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void SetPaused(bool bPaused);

	/** Borra todo lo grabado (por ejemplo, al reaparecer). */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void ClearRecording();

	/** Copia de la grabación actual, de la más vieja a la más reciente. La usa el Eco. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	TArray<FTemporalFrame> GetRecording() const;

	UFUNCTION(BlueprintPure, Category = "Entropia")
	float GetRecordedSeconds() const { return RecordedSeconds; }

	/** Último frame aplicado al rebobinar (su pose sirve para animar en reversa). */
	const FTemporalFrame& GetLastRewoundFrame() const { return LastRewoundFrame; }

	bool IsRewinding() const { return State == ETemporalState::Rewinding; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void RecordFrame(float DeltaTime);
	void RewindStep(float DeltaTime);

	/** Frames válidos: de FirstFrame al final (los anteriores ya se olvidaron). */
	int32 NumFrames() const { return Frames.Num() - FirstFrame; }

	UPROPERTY()
	TArray<FTemporalFrame> Frames;

	/** Índice del frame más viejo que todavía se recuerda. */
	int32 FirstFrame = 0;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> PhysicsRoot;

	UPROPERTY()
	TObjectPtr<class USkeletalMeshComponent> PoseSource;

	FTemporalFrame LastRewoundFrame;
	float RecordedSeconds = 0.f;
	float RewindAccumulator = 0.f;
	bool bWasSimulatingPhysics = false;
};
