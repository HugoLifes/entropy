// ENTROPÍA · Árbitro del tiempo (Fase 3)
// Cada poder PIDE una velocidad de tiempo para un actor (o para el mundo entero) y la SUELTA al terminar.
// Gana la más lenta: no se multiplican. Así una bala congelada por una burbuja no se descongela cuando el Ojo lento
// la suelta, y la Sobremarcha junto con el tiempo de reflejo no dejan el mundo a 0.25 × 0.3.
// Es el único que escribe CustomTimeDilation y la dilatación global. En los tiempos lentos globales el jugador
// siempre va a velocidad normal (su propia dilatación los compensa).
// La física de Unreal no respeta CustomTimeDilation: los cuerpos con física se congelan o se frenan a mano aquí.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TemporalDilation.generated.h"

class APawn;

/** Pedidos de velocidad por clave (un actor) y por fuente (un poder). Gana el más lento. Lógica pura, sin mundo. */
struct ENTROPY_API FTemporalDilationLedger
{
	/** Pide (o actualiza) la velocidad de Source para Key. True si cambió la velocidad efectiva de Key. */
	bool Request(const void* Key, const void* Source, float Dilation);

	/** Suelta el pedido de Source para Key. True si cambió la velocidad efectiva. */
	bool Release(const void* Key, const void* Source);

	/** Suelta todo lo que pidió Source. Devuelve las claves cuya velocidad efectiva cambió. */
	TArray<const void*> ReleaseSource(const void* Source);

	/** Velocidad efectiva de Key: la más lenta de sus pedidos, o 1 si no hay ninguno. */
	float Get(const void* Key) const;

	bool Has(const void* Key) const { return Requests.Contains(Key); }
	void RemoveKey(const void* Key) { Requests.Remove(Key); }
	void GetKeys(TArray<const void*>& OutKeys) const { Requests.GetKeys(OutKeys); }

private:
	static float Slowest(const TMap<const void*, float>& BySource);

	TMap<const void*, TMap<const void*, float>> Requests;
};

UCLASS()
class ENTROPY_API UTemporalDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTemporalDilationSubsystem* Get(const UObject* WorldContext);

	/** Por debajo de esto el actor cuenta como congelado (su física se apaga). */
	static constexpr float FrozenBelow = 0.05f;

	/** Velocidad propia del jugador cuando el mundo va a GlobalDilation: así se ve a velocidad normal. */
	static float PlayerTimeFor(float OwnDilation, float GlobalDilation);

	void Request(AActor* Actor, const void* Source, float Dilation);
	void Release(AActor* Actor, const void* Source);
	void ReleaseSource(const void* Source);

	/** Velocidad propia del actor (1 si nadie pide nada). No incluye la dilatación global. */
	float GetDilation(const AActor* Actor) const;
	bool IsFrozen(const AActor* Actor) const { return GetDilation(Actor) < FrozenBelow; }

	/** Tiempo lento para todo el mundo menos el jugador (Sobremarcha, tiempo de reflejo, remate). */
	void RequestGlobal(const void* Source, float WorldDilation);
	void ReleaseGlobal(const void* Source);
	float GetGlobalDilation() const { return Global.Get(this); }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Un cuerpo con física que estamos congelando o frenando a mano. */
	struct FBodyState
	{
		bool bFrozen = false;
		bool bSlowed = false;
		/** Apagamos su simulación al congelarlo (y hay que prenderla al soltarlo). */
		bool bDisabledPhysics = false;
		bool bHadGravity = true;
		/** Velocidad a tiempo normal, guardada al congelarlo. */
		FVector LinearVelocity = FVector::ZeroVector;
		FVector AngularVelocity = FVector::ZeroVector;
	};

	void Apply(AActor* Actor, float OldDilation, float NewDilation);
	void ApplyBody(AActor* Actor, float OldDilation, float NewDilation);
	void ApplyPlayer();
	APawn* GetPlayer() const;
	/** Si la clave era de un actor ya destruido (y la dirección se reusó), se olvida. */
	void ForgetIfStale(AActor* Actor);

	FTemporalDilationLedger Actors;
	FTemporalDilationLedger Global;
	TMap<const void*, TWeakObjectPtr<AActor>> KeyActors;
	TMap<TWeakObjectPtr<AActor>, FBodyState> Bodies;
};
