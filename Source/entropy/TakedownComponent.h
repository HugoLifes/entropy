// ENTROPÍA · Remates (Fase 3)
// Si un maniquí está cerca y (a) estás a su espalda o (b) está congelado, el HUD muestra "CLIC: REMATE".
// Con clic izquierdo: te alineas con él, cámara lenta breve (el mundo a SlowDilation, tú normal), golpe (AM_ComboAttack
// de la plantilla), onda de distorsión y el maniquí sale volando. Si estaba congelado, el golpe se guarda (ver el maniquí).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakedownComponent.generated.h"

class ATemporalMannequin;
class UAnimMontage;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Takedown (Remates)"))
class ENTROPY_API UTakedownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTakedownComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "50"))
	float MaxDistance = 180.f;

	/** Grados desde el frente del maniquí a partir de los cuales cuentas como "a su espalda". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "90", ClampMax = "180"))
	float BehindAngle = 120.f;

	/** Velocidad del mundo durante el remate (tú vas normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.05", ClampMax = "1"))
	float SlowDilation = 0.2f;

	/** Segundos reales de cámara lenta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float SlowSeconds = 0.4f;

	/** Segundos reales desde el clic hasta que el golpe conecta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float HitDelay = 0.12f;

	/** Empujón al maniquí (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float Impulse = 900.f;

	/** Distancia a la que te pones del maniquí para el golpe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float AlignDistance = 95.f;

	/** El maniquí que remataría un clic ahora mismo (nulo si no hay). */
	ATemporalMannequin* GetAvailableTarget() const { return Available.Get(); }

	/** Remata al maniquí disponible. False si no hay ninguno o ya estás rematando. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	bool TryTakedown();

	bool IsBusy() const { return BusyTimer > 0.f; }

	/** Corta el remate en curso (al morir): sin cámara lenta, sin golpe pendiente, sin animación. */
	void Cancel();

	/** ¿Se puede rematar? Cerca y (a su espalda o congelado). Lógica pura, probada. */
	static bool CanTakedown(const FVector& Attacker, const FVector& Target, const FVector& TargetForward, bool bFrozen, float InMaxDistance, float InBehindAngle);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ATemporalMannequin* FindTarget() const;
	void EndSlow();

	UPROPERTY()
	TObjectPtr<UAnimMontage> AttackMontage;

	TWeakObjectPtr<ATemporalMannequin> Available;
	TWeakObjectPtr<ATemporalMannequin> Victim;
	FVector HitDirection = FVector::ForwardVector;
	float BusyTimer = 0.f;
	float Elapsed = 0.f;
	float SlowTimer = 0.f;
	bool bHitLanded = false;
};
