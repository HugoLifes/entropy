// ENTROPÍA · Objeto que se rompe y se rearma (paso 5)
// Un bloque que, al recibir un golpe fuerte (o al llamar Break), se cambia por pedazos con física.
// Cada pedazo tiene su Registro Temporal: si se rebobinan (R) hasta su lugar, el bloque se rearma.
// Las balas invertidas (ATemporalProjectile) llaman RewindShards al pasar por donde estaba.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalBreakable.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AStaticMeshActor;
class UTemporalRecorderComponent;

UCLASS()
class ENTROPY_API ATemporalBreakable : public AActor
{
	GENERATED_BODY()

public:
	ATemporalBreakable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Tamaño del bloque entero (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FVector Size = FVector(100.f, 100.f, 100.f);

	/** En cuántos pedazos se parte por eje. Ojo con el rendimiento: 2x2x2 = 8 pedazos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FIntVector Pieces = FIntVector(2, 2, 2);

	/** Velocidad (cm/s) que debe traer un objeto con física para romperlo al chocar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float BreakSpeed = 700.f;

	/** Qué tan fuerte salen volando los pedazos (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0"))
	float BlastStrength = 450.f;

	/** Segundos que recuerda cada pedazo. Pasado ese tiempo ya no se puede rearmar con R. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1"))
	float ShardMemorySeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FLinearColor Color = FLinearColor(0.75f, 0.7f, 0.6f);

	/** Rompe el bloque. Direction = hacia dónde iba el golpe (empuja los pedazos). */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Break(FVector ImpactPoint, FVector Direction);

	/** Rebobina todos los pedazos a la velocidad dada; al llegar a su sitio, el bloque se rearma solo. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void RewindShards(float Speed = 1.f);

	/** Lo deja entero de golpe (para reiniciar una sala). */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void Restore();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsBroken() const { return bBroken; }

	/** Registros de los pedazos vivos (vacío si está entero). */
	TArray<UTemporalRecorderComponent*> GetShardRecorders() const;

	/** ¿El segmento A→B pasa por donde está (o estaba) el bloque? */
	bool SegmentCrosses(const FVector& A, const FVector& B) const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleShardRewindFinished();

	void Reassemble();
	void DestroyShards();

	UPROPERTY()
	TArray<TObjectPtr<AStaticMeshActor>> Shards;

	TArray<FVector> ShardHomes;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	bool bBroken = false;
	/** Tras rearmarse, unos instantes en los que no se puede volver a romper (el golpe que lo rompió sigue ahí). */
	float Invulnerable = 0.f;
};
