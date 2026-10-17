// ENTROPÍA · Golpes (Fase 3b, estilo Tang Soo Do)
// Clic izquierdo sin apuntar: combo de 3 golpes si vas haciendo clic a tiempo. Los dos primeros empujan y aturden; el
// tercero tumba. Te gira solo hacia el maniquí más cercano enfrente (para no pegarle al aire).
// Las animaciones son las de la plantilla mientras llegan las de artes marciales (DA_Animaciones: Punch1/2/3).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaMeleeComponent.generated.h"

class ACharacter;
class ATemporalMannequin;

/** El combo: qué golpe toca con cada clic. Lógica pura (probada). */
struct ENTROPY_API FEntropiaCombo
{
	/** Si el siguiente clic llega a menos de esto, sigue el combo; si no, vuelve al golpe 1. */
	float Window = 0.5f;
	/** Clics más rápidos que esto se ignoran (todavía estás pegando). */
	float MinGap = 0.25f;
	int32 MaxHits = 3;

	float LastClick = -100.f;
	int32 Index = 0;

	/** Qué golpe sale con un clic en Now (1, 2, 3; 0 = ninguno). */
	int32 Next(float Now);
};

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Melee"))
class ENTROPY_API UEntropiaMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaMeleeComponent();

	/** Alcance del golpe (cm) y ancho del cono enfrente (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float Reach = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float ConeDegrees = 70.f;

	/** Segundos (reales) desde el clic hasta que el golpe conecta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float HitDelay = 0.18f;

	/** Clic izquierdo sin apuntar. False si no se puede golpear ahora. */
	bool TryAttack();

	int32 GetLastIndex() const { return LastIndex; }
	int32 GetHitsLanded() const { return HitsLanded; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	ACharacter* GetCharacter() const;
	ATemporalMannequin* FindTarget(float MaxDistance, float HalfConeDegrees) const;
	void LandHit();

	FEntropiaCombo Combo;
	float Clock = 0.f;
	float PendingHit = -1.f;
	int32 PendingIndex = 0;
	int32 LastIndex = 0;
	int32 HitsLanded = 0;
};
