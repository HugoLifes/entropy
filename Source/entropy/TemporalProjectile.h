// ENTROPÍA · Bala temporal (paso 5)
// Bala normal: vuela en línea recta, rompe los ATemporalBreakable y los atraviesa, empuja lo que tiene física
// y se incrusta en la primera pared. Tiene Registro Temporal: con R sale de la pared y vuelve al cañón.
// Bala invertida (estilo Tenet): nace incrustada en la pared, espera un instante y vuela HACIA ATRÁS hasta el
// cañón; al pasar por un bloque roto, lo rearma.
// Tiene traslape: la burbuja de Brecha la frena o la congela en el aire.
// Fase 3: las balas de las torretas que te siguen (bHurtsPlayer) te pegan: quitan vida y te empujan. Si eres
// invulnerable (dash) te atraviesan.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporalProjectile.generated.h"

class UStaticMeshComponent;
class UTemporalRecorderComponent;

UCLASS(NotPlaceable)
class ENTROPY_API ATemporalProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATemporalProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entropia")
	TObjectPtr<UTemporalRecorderComponent> Recorder;

	/** cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float Speed = 2500.f;

	/** Distancia máxima antes de desaparecer si no choca con nada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float Range = 4000.f;

	/** Cambio de velocidad (cm/s) que reciben los objetos con física al ser atravesados. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float PushSpeed = 400.f;

	/** Segundos que la bala invertida espera incrustada antes de salir hacia el cañón. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float InvertedDelay = 0.6f;

	/** ¿Le pega al jugador? (solo las de las torretas de la sala 5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Combate")
	bool bHurtsPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Combate")
	float Damage = 20.f;

	/** Empujón al jugador cuando le pega (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia|Combate")
	float PlayerPush = 350.f;

	/** Velocidad con la que se mueve de verdad ahora (con su tiempo propio y el del mundo). Cero si está quieta o rebobinándose. */
	FVector GetRealVelocity() const;

	/** Bala normal desde el cañón. */
	void Launch(const FVector& InMuzzle, const FVector& Direction, AActor* InGun);

	/** Bala invertida: nace en Impact y regresa a InMuzzle. */
	void LaunchInverted(const FVector& InMuzzle, const FVector& Impact, AActor* InGun);

	UFUNCTION(BlueprintPure, Category = "Entropia")
	bool IsInverted() const { return State == EState::InvertedWaiting || State == EState::InvertedFlying; }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	enum class EState : uint8
	{
		Idle,
		Flying,
		Embedded,
		InvertedWaiting,
		InvertedFlying
	};

	UFUNCTION()
	void HandleRewindFinished();

	void StepForward(float DeltaTime);
	void StepInverted(float DeltaTime);
	void Embed(const FVector& At);
	void ReturnToGun();
	void ResetIgnored();
	void Paint(const FLinearColor& Color);
	/** Le pega al jugador (vida y empujón). False si no hubo daño (invulnerable): entonces lo atraviesa. */
	bool HitPlayer(class ACharacter* Victim, const FVector& Where);

	EState State = EState::Idle;
	FVector Muzzle = FVector::ZeroVector;
	FVector Dir = FVector::ForwardVector;
	float Traveled = 0.f;
	float Wait = 0.f;
	TWeakObjectPtr<AActor> Gun;
	/** Lo que ya atravesó en este vuelo (bloques rotos, cajas empujadas). */
	TArray<TWeakObjectPtr<AActor>> Pierced;
};
