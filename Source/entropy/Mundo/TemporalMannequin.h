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

	/** Un golpe normal: lo empuja y lo aturde un momento (bKnockdown: lo tumba como el remate). */
	virtual void ReceiveHit(const FVector& Direction, bool bKnockdown);

	/** Aturdido por un golpe (no hace nada durante unos instantes de su tiempo). */
	bool IsStaggered() const { return StaggerTime > 0.f; }

	/** Ocupado reaccionando: tirado, con golpe pendiente, aturdido, cayendo del cuello roto o sometido. */
	bool IsReacting() const { return bKnockedDown || bPendingHit || StaggerTime > 0.f || NeckBreakTime > 0.f || bSubdued; }

	/** Romperle el cuello: se desploma (animación de muerte y luego muñeco de trapo). Congelado, espera a que vuelva su tiempo. */
	void ReceiveNeckBreak(const FVector& Direction);

	/** Sometido (agarrado del brazo): no hace nada hasta que lo sueltan. PoseDelay: segundos (de su tiempo) antes de la pose
	 *  de la llave, para que se vea la animación que ya está haciendo (el desarme). */
	bool IsSubdued() const { return bSubdued; }
	void SetSubdued(bool bInSubdued, float PoseDelay = 0.f);

	/** Desplomándose con el cuello roto (o con el crujido guardado mientras está congelado). */
	bool IsCollapsing() const { return NeckBreakTime > 0.f || bPendingNeckBreak; }

	bool IsKnockedDown() const { return bKnockedDown; }
	bool HasPendingHit() const { return bPendingHit; }

	/** De pie en su lugar, sin golpes pendientes (al reiniciar la sala). */
	virtual void ResetMannequin();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void KnockDown(const FVector& Direction, float Impulse);

	/** Segundos (de su tiempo) que aguanta aturdido tras un golpe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float StaggerSeconds = 0.6f;

	float StaggerTime = 0.f;
	float NeckBreakTime = 0.f;
	FVector NeckBreakDirection = FVector::ForwardVector;
	bool bSubdued = false;
	float SubduedPoseDelay = 0.f;
	bool bPendingNeckBreak = false;

private:
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
