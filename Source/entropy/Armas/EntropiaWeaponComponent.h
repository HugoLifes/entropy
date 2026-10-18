// ENTROPÍA · El arma del jugador (Fase 4)
// Revólver (6 tiros, lento, pega fuerte) y fusil (automático, 30 tiros). Apuntando, clic izquierdo dispara balas de verdad
// (ATemporalProjectile: el tiempo las puede congelar, rebobinar y devolver). Retroceso, dispersión, fogonazo, marca de
// impacto, recarga (R sin nada en la mira) y cambio de arma (rueda).
// Jugadas con el tiempo:
//   - Recarga inversa: una bala tuya rebobinada hasta el cañón vuelve al cargador de su arma.
//   - Último en caer: si tu bala tumba al último dummy armado de pie de la sala, el mundo va lento un momento y la
//     cámara se acerca.
// Los números y los modelos están en DA_Armas (UEntropiaWeaponSet); las animaciones en DA_Animaciones.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntropiaGuns.h"
#include "EntropiaWeaponComponent.generated.h"

class ATemporalProjectile;
class UStaticMeshComponent;
class UTemporalAimComponent;
class UEntropiaWeaponSet;
struct FEntropiaAnimSlot;
struct FEntropiaWeaponStats;
enum class EEntropiaBulletEvent : uint8;

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Entropia Weapons"))
class ENTROPY_API UEntropiaWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEntropiaWeaponComponent();

	/** 0 = revólver, 1 = fusil. */
	static constexpr int32 NumWeapons = 2;

	/** Último en caer: velocidad del mundo y segundos reales. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float KillCamDilation = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float KillCamSeconds = 0.7f;

	/** Grados por segundo que la cámara regresa después del retroceso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float RecoilRecovery = 8.f;

	/** Clic izquierdo apuntando. Con el arma vacía, recarga. */
	void TriggerPressed();
	void TriggerReleased();

	/** False si ya está lleno o recargando. */
	bool Reload();

	/** Rueda del ratón: +1 / -1. Cancela la recarga. */
	void CycleWeapon(int32 Direction);

	int32 GetCurrentIndex() const { return Current; }
	const FEntropiaWeaponStats& StatsOf(int32 Index) const;
	const FEntropiaWeaponStats& CurrentStats() const { return StatsOf(Current); }
	const FEntropiaWeaponState& StateOf(int32 Index) const { return States[FMath::Clamp(Index, 0, NumWeapons - 1)]; }
	const FEntropiaWeaponState& CurrentState() const { return StateOf(Current); }

	/** Grados de dispersión ahora (para la mira). */
	float GetSpreadDegrees() const;

	/** Marca de impacto en la mira: 1 recién pegaste, 0 ya se fue. Roja si lo tumbaste. */
	float GetHitMarkerAlpha() const;
	bool LastMarkerWasKill() const { return bMarkerKill; }

	/** "+1" de la Recarga inversa (1 recién regresó una bala). */
	float GetRefundAlpha() const;

	int32 NumLiveBullets() const;
	const TArray<TWeakObjectPtr<ATemporalProjectile>>& GetLiveBullets() const { return LiveBullets; }
	int32 GetTotalShots() const { return TotalShots; }

	bool IsKillCamActive() const { return KillCamLeft > 0.f; }

	/** Punta del cañón en el mundo. */
	FVector GetMuzzleLocation() const;

	/** Al morir o teletransportarte: suelta el gatillo, cancela la recarga, quita la cámara lenta y la pose. */
	void CancelAll();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EPose : uint8 { Aim, Fire, Reload };

	bool FireOnce(bool bJustPressed);
	/** Lo que está en el centro de la pantalla (hasta 100 m). */
	FVector FindAimPoint(FVector& OutViewDirection) const;
	void BuildVisual();
	void HandleBulletEvent(ATemporalProjectile* Bullet, EEntropiaBulletEvent Event, AActor* Victim);
	bool IsLastStanding(const AActor* Victim) const;
	void StartKillCam();
	void StopKillCam();
	bool WantsPose() const;
	const FEntropiaAnimSlot* PoseSlot(EPose Pose, int32 Weapon) const;
	void UpdatePose(float DeltaTime);
	void PlayOneShot(EPose Pose);
	void StopOwnPose();

	FEntropiaWeaponState States[NumWeapons];
	int32 Current = 0;
	bool bTriggerHeld = false;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	/** DA_Armas (referencia fuerte mientras exista el jugador). */
	UPROPERTY()
	TObjectPtr<UEntropiaWeaponSet> WeaponSet;

	TWeakObjectPtr<UTemporalAimComponent> Aim;
	TArray<TWeakObjectPtr<ATemporalProjectile>> LiveBullets;

	float RecoilDebt = 0.f;
	float HitMarkerLeft = 0.f;
	bool bMarkerKill = false;
	float RefundLeft = 0.f;
	float KillCamLeft = 0.f;
	bool bAimPosePlaying = false;
	float OneShotLeft = 0.f;
	int32 TotalShots = 0;
};
