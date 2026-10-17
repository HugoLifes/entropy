// ENTROPÍA · Placa de presión + puerta (puzzle básico de Eco)
// La puerta se abre mientras alguien (el jugador o un Eco) está sobre la placa.
// Solución típica: graba un Eco parado en la placa y pasa tú mientras él la mantiene.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlateDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(meta = (DisplayName = "Pressure Plate Door"))
class ENTROPY_API APressurePlateDoor : public AActor
{
	GENERATED_BODY()

public:
	APressurePlateDoor();

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Dónde queda la puerta respecto a la placa (cm). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector DoorOffset = FVector(0.f, 700.f, 150.f);

	/** Cuánto sube la puerta al abrirse (cm). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float OpenHeight = 320.f;

	UPROPERTY(EditAnywhere, Category = "Entropia")
	float OpenSpeed = 2.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Entropia")
	bool bPressed = false;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PlateMaterial;

	float OpenAlpha = 0.f;
};
