// ENTROPÍA · Agarrar, desarmar y someter (Fase 3b, defensa personal del Tang Soo Do)
// E de cerca: le tomas el brazo a un dummy. Si está armado, la pistola cae al piso. Queda sometido (de espaldas a ti)
// unos segundos: clic = romperle el cuello; E otra vez = soltarlo con un empujón. Si no haces nada, se suelta solo.
// Funciona también con dummies congelados.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaGrabComponent.generated.h"

class ACharacter;
class ATemporalMannequin;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Grab"))
class ENTROPY_API UEntropiaGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaGrabComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float MaxDistance = 150.f;
	/** Segundos (reales) que lo tienes sometido antes de que se suelte solo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float HoldSeconds = 4.f;
	/** Distancia a la que lo sostienes enfrente de ti. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float HoldDistance = 70.f;

	/** ¿Se puede agarrar? Enfrente de ti (cono de 60°), cerca y de pie. Lógica pura (probada). */
	static bool CanGrab(const FVector& Attacker, const FVector& AttackerForward, const FVector& Target, bool bTargetStanding, float InMaxDistance = 150.f);

	/** E: agarra al dummy más cercano enfrente. False si no hay ninguno. */
	bool TryGrab();

	/** Lo sueltas (bShove: con un empujón que lo aturde). */
	void Release(bool bShove);

	bool IsHolding() const { return Held.IsValid(); }

	/** ¿Hay alguien que agarrar ahora mismo? (para el aviso "E: AGARRAR") */
	bool HasGrabbable() const { return bHasGrabbable; }
	ATemporalMannequin* GetHeld() const { return Held.Get(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	ACharacter* GetCharacter() const;
	void PlaceHeld();
	ATemporalMannequin* FindGrabbable() const;

	TWeakObjectPtr<ATemporalMannequin> Held;
	float HoldTimer = 0.f;
	bool bHasGrabbable = false;
};
