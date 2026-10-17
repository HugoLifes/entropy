// ENTROPÍA · Brecha
// Una esfera donde el tiempo corre distinto: 0 = congelado, 0.2 = cámara lenta, 3 = acelerado.
// Trae su propia esfera translúcida (Visual) que crece al aparecer y se encoge al terminar.

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

	struct FFrozenBody
	{
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocity = FVector::ZeroVector;
	};

	/** Cuerpo con física en cámara lenta (o acelerado): le quitamos la gravedad y le aplicamos una escalada. */
	struct FSlowedBody
	{
		bool bHadGravity = true;
	};

	float Age = 0.f;

	TSet<TWeakObjectPtr<AActor>> Affected;
	TMap<TWeakObjectPtr<AActor>, FFrozenBody> FrozenBodies;
	TMap<TWeakObjectPtr<AActor>, FSlowedBody> SlowedBodies;

public:
	virtual void Tick(float DeltaTime) override;
};
