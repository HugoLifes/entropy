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

/** Un campo de tiempo detenido (una burbuja): todo lo que quede adentro se ve "congelado". */
struct FTemporalField
{
	uint32 Id = 0;
	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	/** 0..1: qué tan congelado se ve (1 = tiempo detenido, menos = cámara lenta). */
	float Strength = 1.f;
	/** Orden de llegada (para soltar el más viejo si hay demasiados). */
	uint64 Serial = 0;
};

/** Lógica pura de los campos (probada sin mundo). El material tiene lugar para 2. */
struct ENTROPY_API FTemporalFieldSet
{
	static constexpr int32 MaxFields = 2;
	TArray<FTemporalField> Fields;
	uint64 NextSerial = 1;

	/** Agrega o actualiza el campo de ese dueño. Si ya hay 2 de otros dueños, reemplaza el más viejo. */
	void Set(uint32 Id, const FVector& Center, float Radius, float Strength);
	void Remove(uint32 Id);
};

UCLASS()
class ENTROPY_API UTemporalFXSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Atajo: el subsystem del mundo de ese objeto (nulo fuera de Play). */
	static UTemporalFXSubsystem* Get(const UObject* WorldContext);

	/** Onda en un punto del mundo. Strength < 0 = hacia adentro. Duration en segundos reales. */
	void Pulse(const FVector& WorldLocation, float Strength, float Duration = 0.6f, float MaxRadius = 0.35f);

	/** Aura de distorsión siguiendo a un actor (Strength 0 o Target nulo = apagar). */
	void SetAura(AActor* Target, float Strength);

	/**
	 * Campo de tiempo detenido de una esfera del mundo (la burbuja). Lo de adentro se ve desaturado y frío,
	 * y el borde dobla la imagen. Owner identifica el campo: llamarlo cada frame lo actualiza.
	 */
	void SetField(const UObject* Owner, const FVector& Center, float Radius, float Strength);
	void ClearField(const UObject* Owner);

	/** Un material sólido compartido por color (las balas: el fusil dispara 10 por segundo y no hace falta uno por bala). */
	UMaterialInstanceDynamic* GetSharedSolid(const FLinearColor& Color);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void ApplyToMaterial(bool& bOutAnyActive);

	FTemporalPulseSet Pulses;
	FTemporalFieldSet Fields;
	TWeakObjectPtr<AActor> AuraTarget;
	float AuraStrength = 0.f;
	/** Última fuerza pedida: el aura se apaga suave desde ahí. */
	float AuraFadeStrength = 0.f;
	/** Entrada y salida suave del aura (0..1). */
	float AuraBlend = 0.f;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY()
	TMap<uint32, TObjectPtr<UMaterialInstanceDynamic>> SharedSolids;

	UPROPERTY()
	TObjectPtr<AActor> HostActor;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcess;
};
