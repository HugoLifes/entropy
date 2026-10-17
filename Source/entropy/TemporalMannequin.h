// ENTROPÍA · Maniquí temporal (Fase 3)
// Un clon oscuro del jugador (mismo cuerpo y animación) para practicar remates. Está de pie; con bLookAround mira a
// los lados. La burbuja y Detener lo congelan de verdad (su tiempo propio vale 0: no se mueve ni se anima).
// Al rematarlo cae como muñeco de trapo y, a los RecoverSeconds de SU tiempo, "el tiempo lo regresa": aparece de pie
// en su lugar con una onda. Si lo rematas congelado, el golpe se guarda y sale cuando su tiempo vuelve a correr.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TemporalMannequin.generated.h"

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalMannequin : public ACharacter
{
	GENERATED_BODY()

public:
	ATemporalMannequin();

	/** Segundos (de su tiempo) tirado antes de volver a su lugar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float RecoverSeconds = 4.f;

	/** Mira a los lados (como un vigía): así la espalda no siempre está en el mismo lugar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bLookAround = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float LookAroundDegrees = 70.f;

	/** Grados por segundo (como máximo) al girar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float LookAroundSpeed = 30.f;

	/** Cuerpo y animación del jugador, en colores oscuros (para distinguirlo de ti y de tus ecos). */
	void SetupFromPlayer(USkeletalMeshComponent* PlayerMesh);

	/** Recibe un remate. Si está congelado, el golpe espera a que vuelva su tiempo. */
	void ReceiveTakedown(const FVector& Direction, float Impulse);

	bool IsKnockedDown() const { return bKnockedDown; }
	bool HasPendingHit() const { return bPendingHit; }

	/** De pie en su lugar, sin golpes pendientes (al reiniciar la sala). */
	void ResetMannequin();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void KnockDown(const FVector& Direction, float Impulse);
	void Recover(bool bWithEffect);
	/** Congela o suelta el muñeco de trapo (la física del cuerpo no respeta CustomTimeDilation). */
	void SetRagdollFrozen(bool bFrozen);

	/** Velocidades de cada hueso al congelarse el muñeco de trapo (se devuelven al soltarlo). */
	TArray<FVector> FrozenLinear;
	TArray<FVector> FrozenAngular;
	bool bRagdollFrozen = false;

	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;
	FVector MeshRelativeLocation = FVector(0.f, 0.f, -96.f);
	FRotator MeshRelativeRotation = FRotator(0.f, -90.f, 0.f);

	FVector PendingDirection = FVector::ForwardVector;
	float PendingImpulse = 0.f;
	bool bPendingHit = false;
	bool bKnockedDown = false;
	float DownTime = 0.f;
	float LookTime = 0.f;
};
