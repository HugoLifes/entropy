// ENTROPÍA · Efectos visuales compartidos
// Usa contenido del motor y un único asset propio que el editor crea solo:
//   - Material translúcido: /Game/Entropia/Materiales/M_TemporalGhost (se crea al abrir el editor si falta;
//     parámetro "Color", alfa = opacidad). Respaldo: /Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.
//   - Material sólido:      /Engine/BasicShapes/BasicShapeMaterial (parámetro "Color")
//   - Partículas:           /Niagara/DefaultAssets/Templates/Systems/RadialBurst
// Cuando haya arte propio, cambia las rutas aquí y todo el juego se actualiza.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalFX.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UPoseableMeshComponent;
class USkinnedAsset;

namespace TemporalFX
{
	ENTROPY_API UMaterialInstanceDynamic* MakeTranslucent(UObject* Outer, const FLinearColor& Color);
	ENTROPY_API UMaterialInstanceDynamic* MakeSolid(UObject* Outer, const FLinearColor& Color);
	ENTROPY_API void ApplyToAllSlots(UMeshComponent* Mesh, UMaterialInterface* Material);
	/** Estallido de partículas (Niagara). No hace nada si la plantilla no carga. */
	ENTROPY_API void Burst(const UObject* WorldContext, const FVector& Location, const FLinearColor& Color, float Scale = 1.f);

	/** Busca un componente en el actor y, si no existe, lo crea y lo registra (así cada poder funciona solo). */
	template <typename T>
	T* FindOrAddComponent(AActor* Owner, const TCHAR* Name)
	{
		T* Component = Owner->FindComponentByClass<T>();
		if (!Component)
		{
			Component = NewObject<T>(Owner, Name);
			Component->RegisterComponent();
		}
		return Component;
	}
}

/** Copia translúcida de una pose que se desvanece (estela del Paso Inverso). */
UCLASS(NotPlaceable)
class ENTROPY_API ATemporalAfterimage : public AActor
{
	GENERATED_BODY()

public:
	ATemporalAfterimage();

	void Init(USkinnedAsset* Asset, const FTransform& MeshWorldTransform, const TArray<FTransform>& Pose, const FLinearColor& Color, float LifeSeconds);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UPoseableMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	FLinearColor BaseColor = FLinearColor::White;
	float Life = 0.4f;
	float Age = 0.f;
};
