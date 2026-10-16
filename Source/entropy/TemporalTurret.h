// ENTROPÍA · Torreta de pruebas (paso 5)
// Dispara hacia su frente (+X). Fire = bala normal · FireInverted = bala invertida (nace en la pared y regresa).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalTurret.generated.h"

class UStaticMeshComponent;
class ATemporalProjectile;

UCLASS()
class ENTROPY_API ATemporalTurret : public AActor
{
	GENERATED_BODY()

public:
	ATemporalTurret();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Barrel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float BulletSpeed = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float Range = 4000.f;

	/** Balas vivas a la vez; al disparar una más desaparece la más vieja. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1"))
	int32 MaxBullets = 6;

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Fire();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void FireInverted();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void ClearBullets();

protected:
	virtual void BeginPlay() override;

private:
	ATemporalProjectile* SpawnBullet(const FVector& Location);

	TArray<TWeakObjectPtr<ATemporalProjectile>> Bullets;
};
