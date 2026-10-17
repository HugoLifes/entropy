// ENTROPÍA · Estela ligera (Fase 1)
// Copias translúcidas del cuerpo que se desvanecen. A diferencia de ATemporalAfterimage (un actor nuevo por copia),
// aquí hay un POOL FIJO de mallas que se reusan: crear y destruir actores y materiales cada 0.06 s bajaba los FPS.
// Lo usan el Paso Inverso (F) y la Sobremarcha (V). Se desvanece con el DeltaTime del dueño (tiempo real del jugador).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalTrailComponent.generated.h"

class UPoseableMeshComponent;
class UMaterialInstanceDynamic;
class USkinnedAsset;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Trail"))
class ENTROPY_API UTemporalTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalTrailComponent();

	/** Cuántas copias puede haber a la vez. Al emitir una más se reusa la más vieja. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "1", ClampMax = "12"))
	int32 PoolSize = 4;

	/** Segundos que tarda cada copia en desaparecer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.05"))
	float Life = 0.30f;

	/** Deja una copia con esta pose (espacio de hueso) en esta posición del mundo. */
	void Emit(USkinnedAsset* Asset, const TArray<FTransform>& Pose, const FTransform& MeshWorldTransform, const FLinearColor& Color);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FSlot
	{
		TWeakObjectPtr<UPoseableMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		FLinearColor Color = FLinearColor::White;
		float Age = 0.f;
		bool bActive = false;
	};

	void BuildPool(USkinnedAsset* Asset);

	TArray<FSlot> Slots;

	/** Mantiene vivos los componentes y materiales del pool. */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> Keep;
};
