// ENTROPÍA · Brecha
// Una esfera donde el tiempo corre distinto: 0 = congelado, 0.2 = cámara lenta, 3 = acelerado.
// Trae su propia esfera translúcida (Visual) que crece al aparecer y se encoge al terminar.
// Pide su velocidad al árbitro del tiempo (UTemporalDilationSubsystem): si dos efectos tocan al mismo actor, gana el más lento.
// Con OnlyTarget es la burbujita de "Detener": solo afecta a ese actor y desaparece si él sale.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeBubble.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class ENTROPY_API ATimeBubble : public AActor
{
	GENERATED_BODY()

public:
	ATimeBubble();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	FLinearColor VisualColor = FLinearColor(0.3f, 0.85f, 1.f, 0.55f);

	/** Velocidad del tiempo dentro de la burbuja. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.0"))
	float Dilation = 0.f;

	/** Cuántos segundos dura la burbuja (0 = para siempre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.0"))
	float LifeSeconds = 5.f;

	/** ¿También afecta al jugador? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bAffectPlayer = false;

	/** Si se pone (antes de terminar de aparecer), solo afecta a este actor; si él sale o desaparece, la burbuja también. */
	TWeakObjectPtr<AActor> OnlyTarget;

	/** Distorsión de pantalla dentro de la burbuja (las burbujitas de Detener no la usan: solo hay 2 lugares). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|FX")
	bool bDrawField = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void Affect(AActor* Actor);
	void Release(AActor* Actor);

	float Age = 0.f;
	bool bHasOnlyTarget = false;

	TSet<TWeakObjectPtr<AActor>> Affected;

public:
	virtual void Tick(float DeltaTime) override;
};
