// ENTROPÍA · Pedazo irregular de un objeto roto (Fase 2)
// Una celda de la fractura Voronoi convertida en malla procedural con física y Registro Temporal.
// Caras de afuera con el color del objeto y caras del corte (interior) con otro color.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalShard.generated.h"

class UProceduralMeshComponent;
class UTemporalRecorderComponent;
class UMaterialInterface;
struct FFractureCell;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalShard : public AActor
{
	GENERATED_BODY()

public:
	ATemporalShard();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UTemporalRecorderComponent> Recorder;

	/**
	 * Construye la malla y la colisión convexa de la celda. Los vértices quedan relativos a CellCentroid
	 * (el actor debe estar ahí). Shrink < 1 encoge el pedazo hacia su centro para que no choque con sus vecinos al nacer.
	 */
	void Build(const FFractureCell& Cell, const FVector& CellCentroid, UMaterialInterface* OuterMaterial, UMaterialInterface* InnerMaterial,
		float MemorySeconds, float Shrink = 0.97f);
};
