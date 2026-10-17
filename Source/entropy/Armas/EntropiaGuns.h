// ENTROPÍA · Lógica pura de las armas (Fase 4, probada)
// El cargador, la cadencia, la recarga y la dispersión de un arma; dónde pega una bala (cabeza o torso); cuánto avanza
// una bala del jugador con el mundo detenido ("lluvia detenida"); y cuántas balas viejas quitar. Sin mundo ni actores.

#pragma once

#include "CoreMinimal.h"

struct FEntropiaWeaponStats;

/** El estado de un arma del jugador. Cuenta con el tiempo del jugador. */
struct ENTROPY_API FEntropiaWeaponState
{
	int32 Ammo = 0;
	/** Segundos hasta poder disparar otra vez. */
	float Cooldown = 0.f;
	/** Segundos que faltan de recarga (0 = no está recargando). */
	float ReloadLeft = 0.f;
	/** Dispersión extra por disparar seguido (grados). */
	float Bloom = 0.f;
	/** Segundos desde el último tiro (la dispersión empieza a cerrarse un momento después). */
	float TimeSinceShot = 100.f;

	void Init(const FEntropiaWeaponStats& Stats);

	/** Gasta una bala si se puede. Las semiautomáticas solo disparan al presionar (bJustPressed). */
	bool TryFire(const FEntropiaWeaponStats& Stats, bool bTriggerHeld, bool bJustPressed);

	void Tick(const FEntropiaWeaponStats& Stats, float Dt);

	/** False si ya está lleno o ya está recargando. */
	bool StartReload(const FEntropiaWeaponStats& Stats);
	void CancelReload() { ReloadLeft = 0.f; }
	bool IsReloading() const { return ReloadLeft > 0.f; }

	/** Recarga inversa: balas que regresan al cargador. Devuelve cuántas entraron (nunca pasa del máximo). */
	int32 Refund(const FEntropiaWeaponStats& Stats, int32 Count);

	/** Grados del cono de dispersión ahora. */
	float SpreadDegrees(const FEntropiaWeaponStats& Stats, bool bMoving) const;
};

namespace EntropiaGuns
{
	/** Los 30 cm de arriba de la cápsula cuentan como cabeza. */
	constexpr float HeadHeight = 30.f;
	/** Más que esto vivas y desaparece la más vieja. */
	constexpr int32 MaxLiveBullets = 40;
	/** Los primeros cm de una bala del jugador van en tiempo real (aunque el mundo esté detenido). */
	constexpr float FreeTravel = 150.f;

	ENTROPY_API bool IsHeadshot(float ImpactZ, float CapsuleCenterZ, float HalfHeight);

	/** Tiros de este daño que hacen falta para tumbar a alguien con este aguante. */
	ENTROPY_API int32 ShotsToKnockDown(float Damage, float Toughness);

	/**
	 * Cuánto avanza una bala en este frame. Dt = su tiempo (con dilatación), RealDt = el tiempo real.
	 * Mientras Traveled < FreeTravel avanza en tiempo real; después, con su tiempo.
	 */
	ENTROPY_API float StepDistance(float Speed, float Dt, float RealDt, float Traveled, float FreeTravel);

	/** Cuántas balas viejas hay que quitar para no pasar del tope. */
	ENTROPY_API int32 OldestToEvict(int32 NumLive, int32 Cap);
}
