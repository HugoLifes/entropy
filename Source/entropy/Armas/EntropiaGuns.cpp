// ENTROPÍA · Lógica pura de las armas

#include "EntropiaGuns.h"
#include "EntropiaWeaponSet.h"

void FEntropiaWeaponState::Init(const FEntropiaWeaponStats& Stats)
{
	Ammo = Stats.MagazineSize;
	Cooldown = 0.f;
	ReloadLeft = 0.f;
	Bloom = 0.f;
	TimeSinceShot = 100.f;
}

bool FEntropiaWeaponState::TryFire(const FEntropiaWeaponStats& Stats, bool bTriggerHeld, bool bJustPressed)
{
	const bool bWantsShot = bJustPressed || (Stats.bAutomatic && bTriggerHeld);
	if (!bWantsShot || IsReloading() || Ammo <= 0 || Cooldown > KINDA_SMALL_NUMBER)
	{
		return false;
	}
	--Ammo;
	// Se suma (no se reemplaza) para que la cadencia no pierda tiempo entre frames.
	Cooldown = FMath::Max(Cooldown, -Stats.FireInterval) + Stats.FireInterval;
	Bloom = FMath::Min(Bloom + Stats.SpreadPerShot, FMath::Max(Stats.SpreadMax - Stats.SpreadMin, 0.f));
	TimeSinceShot = 0.f;
	return true;
}

void FEntropiaWeaponState::Tick(const FEntropiaWeaponStats& Stats, float Dt)
{
	if (Dt <= 0.f)
	{
		return;
	}
	Cooldown = FMath::Max(Cooldown - Dt, -Stats.FireInterval);

	// La dispersión se empieza a cerrar un poco después del último tiro (si no, el fusil nunca se abriría).
	const float RecoverAfter = Stats.FireInterval + 0.1f;
	const float Before = TimeSinceShot;
	TimeSinceShot += Dt;
	const float RecoverTime = FMath::Max(0.f, TimeSinceShot - FMath::Max(Before, RecoverAfter));
	Bloom = FMath::Max(0.f, Bloom - Stats.SpreadRecovery * RecoverTime);

	if (ReloadLeft > 0.f)
	{
		ReloadLeft -= Dt;
		if (ReloadLeft <= 0.f)
		{
			ReloadLeft = 0.f;
			Ammo = Stats.MagazineSize;
		}
	}
}

bool FEntropiaWeaponState::StartReload(const FEntropiaWeaponStats& Stats)
{
	if (IsReloading() || Ammo >= Stats.MagazineSize)
	{
		return false;
	}
	ReloadLeft = Stats.ReloadSeconds;
	return true;
}

int32 FEntropiaWeaponState::Refund(const FEntropiaWeaponStats& Stats, int32 Count)
{
	const int32 Added = FMath::Clamp(Stats.MagazineSize - Ammo, 0, FMath::Max(Count, 0));
	Ammo += Added;
	return Added;
}

float FEntropiaWeaponState::SpreadDegrees(const FEntropiaWeaponStats& Stats, bool bMoving) const
{
	return Stats.SpreadMin + Bloom + (bMoving ? 1.f : 0.f);
}

void FEntropiaStoredShots::Add(const FVector& Direction, float InDamage, float InImpulse)
{
	DirectionSum += Direction.GetSafeNormal();
	Damage += InDamage;
	Impulse += InImpulse;
	++Count;
}

void FEntropiaStoredShots::Clear()
{
	*this = FEntropiaStoredShots();
}

FVector FEntropiaStoredShots::AverageDirection() const
{
	return DirectionSum.GetSafeNormal();
}

namespace EntropiaGuns
{
	bool IsHeadshot(float ImpactZ, float CapsuleCenterZ, float HalfHeight)
	{
		return ImpactZ >= CapsuleCenterZ + HalfHeight - HeadHeight;
	}

	int32 ShotsToKnockDown(float Damage, float Toughness)
	{
		return Damage > 0.f ? FMath::Max(1, FMath::CeilToInt(Toughness / Damage)) : MAX_int32;
	}

	float StepDistance(float Speed, float Dt, float RealDt, float Traveled, float InFreeTravel)
	{
		const float Normal = Speed * FMath::Max(Dt, 0.f);
		const float Free = FMath::Max(0.f, InFreeTravel - Traveled);
		const float RealStep = Speed * FMath::Max(RealDt, 0.f);
		if (Free <= 0.f || RealStep <= Normal)
		{
			return Normal;
		}
		// Primero el tramo libre en tiempo real; el resto del frame, con su propio tiempo.
		const float FreePart = FMath::Min(Free, RealStep);
		const float UsedFraction = FreePart / RealStep;
		return FreePart + (1.f - UsedFraction) * Normal;
	}

	float RecoilApplied(float Pitch, float Kick, float MaxPitch)
	{
		// La cámara no sube más allá del tope: lo que no cabe no se "debe" (si no, al regresar quedaría más abajo).
		return FMath::Clamp(MaxPitch - FRotator::NormalizeAxis(Pitch), 0.f, FMath::Max(Kick, 0.f));
	}

	int32 OldestToEvict(int32 NumLive, int32 Cap)
	{
		return FMath::Max(0, NumLive - Cap);
	}
}
