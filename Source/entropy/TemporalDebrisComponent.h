// ENTROPÍA · Escombros y polvo reversibles (Fase 2)
// Astillas y nubes de polvo que salen al romper un objeto. Su movimiento es ANALÍTICO: la posición es una fórmula
// del tiempo T (no una simulación), así que se puede reproducir hacia atrás exacto. Mientras los pedazos del objeto
// se rebobinan, T baja y el polvo regresa al punto del golpe justo cuando el objeto se rearma.
// Todo se dibuja con dos InstancedStaticMesh (una llamada de dibujo para las astillas y otra para el polvo).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalDebrisComponent.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

struct FDebrisParticle
{
	FVector Origin = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	/** Lado de la astilla o diámetro de la nube (cm). */
	float Size = 8.f;
	/** Grados por segundo de giro (astillas). */
	float SpinDeg = 0.f;
	FVector SpinAxis = FVector::UpVector;
};

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal Debris"))
class ENTROPY_API UTemporalDebrisComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalDebrisComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0", ClampMax = "64"))
	int32 NumChips = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0", ClampMax = "64"))
	int32 NumDust = 10;

	/** Segundos que duran las astillas y el polvo antes de desaparecer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia", meta = (ClampMin = "0.5"))
	float Life = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FLinearColor DustColor = FLinearColor(0.75f, 0.72f, 0.66f, 0.28f);

	/** Lanza astillas y polvo desde el punto del golpe. FloorZ = altura del piso (ahí se quedan las astillas). */
	void Burst(const FVector& ImpactPoint, const FVector& Direction, const FLinearColor& ChipColor, float FloorZ);

	/** Lo esconde todo (al rearmarse el objeto). */
	void Hide();

	/** El objeto avisa cada frame si sus pedazos se están rebobinando (y a qué velocidad). */
	void SetRewinding(bool bInRewinding, float InRewindSpeed = 1.f);

	/** Posición de una astilla en el tiempo T: tiro parabólico que se detiene al tocar el piso. */
	static FVector EvaluateChip(const FDebrisParticle& P, float T, float FloorZ);

	/** Tamaño relativo (0..1) de una nube en el tiempo T: crece rápido, llega al máximo en ~1/4 de su vida y se apaga. */
	static float DustScale(float T, float InLife);

	/** Posición de una nube: se abre hacia afuera y se frena (arrastre del aire). */
	static FVector EvaluateDust(const FDebrisParticle& P, float T);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsureComponents();
	void UpdateInstances();

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ChipMesh;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> DustMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ChipMaterial;

	TArray<FDebrisParticle> Chips;
	TArray<FDebrisParticle> Dust;
	float T = 0.f;
	float FloorHeight = 0.f;
	float RewindSpeed = 1.f;
	bool bActive = false;
	bool bRewinding = false;
};
