// ENTROPÍA · Vida simple

#include "TemporalHealthComponent.h"
#include "TemporalFX.h"

UTemporalHealthComponent::UTemporalHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UTemporalHealthComponent::ApplyDamage(float Amount)
{
	if (bDead || IsInvulnerable() || Amount <= 0.f)
	{
		return false;
	}
	Health = FMath::Max(0.f, Health - Amount);
	SinceDamage = 0.f;
	HurtFlash = 0.35f;
	if (Health <= 0.f)
	{
		bDead = true;
		OnDeath.Broadcast();
	}
	return true;
}

void UTemporalHealthComponent::ResetHealth()
{
	Health = MaxHealth;
	bDead = false;
	SinceDamage = 100.f;
	HurtFlash = 0.f;
}

void UTemporalHealthComponent::Advance(float RealDeltaTime)
{
	InvulnerableTime = FMath::Max(0.f, InvulnerableTime - RealDeltaTime);
	HurtFlash = FMath::Max(0.f, HurtFlash - RealDeltaTime);
	if (bDead)
	{
		return;
	}
	// Solo cuenta el tiempo que ya pasó de los RegenDelay segundos sin daño.
	const float Before = SinceDamage;
	SinceDamage += RealDeltaTime;
	const float RegenTime = FMath::Max(0.f, SinceDamage - FMath::Max(RegenDelay, Before));
	Health = FMath::Min(MaxHealth, Health + RegenPerSecond * RegenTime);
}

void UTemporalHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(TemporalFX::RealDelta());
}
