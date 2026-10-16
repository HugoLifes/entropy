// ENTROPÍA · Prensa del tiempo (puzzle de Brecha)
// Sube y baja sin parar; solo se queda arriba un instante. Si te aplasta, regresas al inicio del cuarto.
// Solución: congélala con la burbuja (Q) mientras está arriba y pasa por debajo.
// Usa el DeltaTime del actor, así que respeta CustomTimeDilation (la burbuja la frena o la detiene).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingHazard.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(meta = (DisplayName = "Moving Hazard"))
class ENTROPY_API AMovingHazard : public AActor
{
	GENERATED_BODY()

public:
	AMovingHazard();

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UBoxComponent> HitBox;

	/** Tamaño de la prensa en cm. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector Size = FVector(300.f, 500.f, 300.f);

	/** Cuánto sube (cm) respecto a su posición de reposo (aplastando el piso). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float Lift = 450.f;

	/** Duración de un ciclo completo (s). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float Period = 1.6f;

	/** Fracción del ciclo que pasa arriba (0.15 = muy poco, imposible pasar sin congelarla). */
	UPROPERTY(EditAnywhere, Category = "Entropia", meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float UpFraction = 0.15f;

	/** A dónde regresa el jugador si lo aplasta (mundo). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector RespawnPoint = FVector::ZeroVector;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	FVector RestLocation;
	float Time = 0.f;
	float Cooldown = 0.f;
};
