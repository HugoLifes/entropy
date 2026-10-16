// ENTROPÍA · Plataforma que se derrumba (puzzle de Inverso)
// Al pisarla, tiembla y cae al vacío. Rebobinándola (R) regresa; si llega a su lugar original queda ANCLADA (azul) y ya no cae.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrumblePlatform.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTemporalRecorderComponent;
class UMaterialInstanceDynamic;

UCLASS(meta = (DisplayName = "Crumble Platform"))
class ENTROPY_API ACrumblePlatform : public AActor
{
	GENERATED_BODY()

public:
	ACrumblePlatform();

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UBoxComponent> StepTrigger;

	UPROPERTY(VisibleAnywhere, Category = "Entropia")
	TObjectPtr<UTemporalRecorderComponent> Recorder;

	/** Tamaño en cm (X, Y, grosor). */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	FVector Size = FVector(220.f, 400.f, 50.f);

	/** Segundos que aguanta después de pisarla. */
	UPROPERTY(EditAnywhere, Category = "Entropia")
	float CrumbleDelay = 0.45f;

	/** Regresa la plataforma a su lugar original, firme. */
	UFUNCTION(BlueprintCallable, Category = "Entropia")
	void ResetPlatform();

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsAnchored() const { return bAnchored; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleRewindFinished();

	void Crumble();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	FTransform HomeTransform;
	bool bCrumbled = false;
	bool bAnchored = false;
	bool bWarning = false;
	float WarningTime = 0.f;
};
