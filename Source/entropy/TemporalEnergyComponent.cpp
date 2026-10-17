// ENTROPÍA · Energía compartida

#include "TemporalEnergyComponent.h"
#include "Misc/App.h"

UTemporalEnergyComponent::UTemporalEnergyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UTemporalEnergyComponent::Spend(float Amount)
{
	if (!CanAfford(Amount))
	{
		return false;
	}
	Energy -= Amount;
	MarkUsed();
	return true;
}

bool UTemporalEnergyComponent::Drain(float PerSecond, float RealDeltaTime)
{
	Energy = FMath::Max(0.f, Energy - PerSecond * RealDeltaTime);
	MarkUsed();
	return Energy > 0.f;
}

void UTemporalEnergyComponent::Advance(float RealDeltaTime)
{
	DenyFlash = FMath::Max(0.f, DenyFlash - RealDeltaTime);
	TimeSinceUse += RealDeltaTime;
	if (TimeSinceUse >= RegenDelay)
	{
		Energy = FMath::Min(MaxEnergy, Energy + RegenPerSecond * RealDeltaTime);
	}
}

void UTemporalEnergyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Ignoramos el DeltaTime recibido: viene dilatado (Sobremarcha, burbujas).
	Advance(FApp::GetDeltaTime());
}
