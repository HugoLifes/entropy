// ENTROPÍA · Torreta de pruebas (paso 5)
// Dispara hacia su frente (+X). Fire = bala normal · FireInverted = bala invertida (nace en la pared y regresa).
// Fase 3: con bTrackPlayer gira hacia el jugador y le dispara cada FireInterval si lo ve (sala 5). Gira y dispara con
// su propio tiempo: congelada (burbuja, Detener) no hace nada. La mira la puede seleccionar (objetivo de tiempo).

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

	/** Sigue al jugador y le dispara (sala 5). Se prende y apaga con SetTrackingEnabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir")
	bool bTrackPlayer = false;

	/** Grados por segundo que gira hacia el jugador. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir")
	float TurnRate = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir", meta = (ClampMin = "0.1"))
	float FireInterval = 1.2f;

	/** Solo dispara si ya apunta al jugador con esta tolerancia (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir")
	float AimTolerance = 8.f;

	/** Segundos antes del primer disparo al empezar a seguirte (así varias torretas no disparan a la vez). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir", meta = (ClampMin = "0"))
	float FirstShotDelay = 1.8f;

	/** Sus balas le pegan al jugador. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Seguir")
	bool bBulletsHurtPlayer = false;

	void SetTrackingEnabled(bool bEnabled);
	bool IsTrackingEnabled() const { return bTrackingEnabled; }

	/** Dispara una bala normal en esa dirección. */
	void FireTowards(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Fire();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void FireInverted();

	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void ClearBullets();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ATemporalProjectile* SpawnBullet(const FVector& Location);

	float FireTimer = 0.f;
	bool bTrackingEnabled = false;

	TArray<TWeakObjectPtr<ATemporalProjectile>> Bullets;
};
