// ENTROPÍA · Distorsión del tiempo ("time shifter" / "refraction time") — Fase 2
// Un solo lugar para el efecto: los poderes piden ondas (anillos que doblan la imagen) y un aura (ondulación tipo
// calor alrededor de algo). Todo se dibuja con UN material de post-process (M_TemporalDistortion, lo genera el editor).
// Sin efectos activos, su peso queda en 0 y no cuesta nada. CVar: entropia.fx.distortion 0 = apagado.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TemporalFXSubsystem.generated.h"

class UMaterialInstanceDynamic;
class UPostProcessComponent;

/** Una onda: un anillo que nace en un punto del mundo y se abre en la pantalla. */
struct FTemporalPulse
{
	FVector World = FVector::ZeroVector;
	float Age = 0.f;
	float Duration = 0.6f;
	/** Radio final en unidades de pantalla (1 = el alto de la pantalla). */
	float MaxRadius = 0.35f;
	/** Cuánto dobla la imagen. Negativa = jala hacia adentro (rebobinar), positiva = empuja hacia afuera. */
	float Strength = 0.02f;
};

/** Lógica pura de las ondas (probada sin mundo). */
struct ENTROPY_API FTemporalPulseSet
{
	static constexpr int32 MaxPulses = 4;
	TArray<FTemporalPulse> Pulses;

	/** Agrega una onda; si ya hay 4, reemplaza la más vieja. */
	void Add(const FTemporalPulse& Pulse);
	/** Envejece las ondas y quita las que ya terminaron. */
	void Advance(float RealDelta);

	static float RadiusAt(const FTemporalPulse& Pulse);
	static float StrengthAt(const FTemporalPulse& Pulse);
};

UCLASS()
class ENTROPY_API UTemporalFXSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Onda en un punto del mundo. Strength < 0 = hacia adentro. Duration en segundos reales. */
	void Pulse(const FVector& WorldLocation, float Strength, float Duration = 0.6f, float MaxRadius = 0.35f);

	/** Aura de distorsión siguiendo a un actor (Strength 0 o Target nulo = apagar). */
	void SetAura(AActor* Target, float Strength);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void ApplyToMaterial(bool& bOutAnyActive);

	FTemporalPulseSet Pulses;
	TWeakObjectPtr<AActor> AuraTarget;
	float AuraStrength = 0.f;
	/** Última fuerza pedida: el aura se apaga suave desde ahí. */
	float AuraFadeStrength = 0.f;
	/** Entrada y salida suave del aura (0..1). */
	float AuraBlend = 0.f;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY()
	TObjectPtr<AActor> HostActor;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcess;
};
