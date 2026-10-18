// ENTROPÍA · Dummy armado (Fase 3b)
// Un maniquí temporal con pistola, cubierto detrás de un muro: cada tanto se asoma de lado, te apunta, dispara dos
// balas (de las que te pegan) y se vuelve a cubrir. Es solo una rutina, sin inteligencia (eso es la Fase 5).
// Congelado (burbuja, Detener) no hace nada. Un golpe lo interrumpe. Si lo agarras, lo desarmas: la pistola cae.

#pragma once

#include "CoreMinimal.h"
#include "TemporalMannequin.h"
#include "TemporalArmedDummy.generated.h"

class UStaticMeshComponent;
class ATemporalProjectile;

/** La rutina de asomarse y disparar. Lógica pura (probada). */
struct ENTROPY_API FArmedDummyRoutine
{
	enum class EStep : uint8
	{
		Covered,
		Stepping,
		Firing,
		Returning
	};

	float CoverSeconds = 2.5f;
	float StepSeconds = 0.3f;
	int32 Shots = 2;
	float ShotInterval = 0.35f;
	/** Lo que se queda afuera después del último disparo. */
	float LingerSeconds = 0.25f;
	float ReturnSeconds = 0.3f;

	EStep Step = EStep::Covered;
	float Time = 0.f;
	int32 ShotsFired = 0;

	/** Avanza el tiempo (el suyo: congelado = 0). Devuelve cuántos disparos tocan ahora. */
	int32 Advance(float Dt);

	/** Qué tan afuera está: 0 = cubierto, 1 = asomado del todo. */
	float StepAlpha() const;

	void Reset();
};

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalArmedDummy : public ATemporalMannequin
{
	GENERATED_BODY()

public:
	ATemporalArmedDummy();

	/** La pistola (el cuerpo del modelo de DA_Armas.DummyPistol, o un cubo si falta). Sus otras piezas cuelgan de ella. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Gun;

	/** ¿Usa el modelo de DA_Armas (true) o el cubo provisional? */
	bool HasGunModel() const { return bGunModel; }

	/** Hacia dónde sale al asomarse (en el mundo; lo pone la sala al crearlo). */
	FVector PeekOffset = FVector(0.f, 150.f, 0.f);

	/** Espera extra antes de su primera asomada (para que no se asomen todos a la vez). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float RoutineDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float BulletSpeed = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float BulletDamage = 20.f;
	/** Balas suyas vivas como máximo (al pasar, desaparece la más vieja; como las torretas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") int32 MaxBullets = 8;

	bool IsArmed() const { return bArmed; }
	int32 GetTotalShots() const { return TotalShots; }

	/** Le quitas la pistola (cae al piso con física). Devuelve cuánto dura su reacción (segundos). */
	float Disarm(const FVector& Direction);

	/** Devuélvesela (Fase 4): su propia bala, rebobinada hasta su cañón, le pega a él. Lo tumba si está de pie y armado. */
	void ReceiveReturnedBullet(const FVector& Direction);

	/** Dispara una bala hacia el jugador si lo ve. */
	void FireAtPlayer();

	virtual void ReceiveHit(const FVector& Direction, bool bKnockdown) override;
	virtual void ResetMannequin() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void RestartRoutine();
	void PlaceGun(bool bAiming);
	/** Pone el modelo de DA_Armas.DummyPistol en la pistola (si está importado). */
	void SetupGunModel();
	/** Punta del cañón en el mundo. */
	FVector MuzzleLocation() const;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> GunExtraParts;
	/** Giro del modelo dentro de la pistola (el cubo apunta a +X; el modelo, a -Y). */
	FQuat GunModelRotation = FQuat::Identity;
	bool bGunModel = false;
	TArray<TWeakObjectPtr<ATemporalProjectile>> MyBullets;

	FArmedDummyRoutine Routine;
	FVector CoverHome = FVector::ZeroVector;
	bool bArmed = true;
	bool bAimPosePlaying = false;
	/** Segundos (de su tiempo) que le quedan a la animación de disparo; luego vuelve la pose de apuntar. */
	float FireAnimLeft = 0.f;
	int32 TotalShots = 0;
};
