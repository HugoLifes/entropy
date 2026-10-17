// ENTROPÍA · Invertir objeto

#include "ObjectInversionComponent.h"
#include "TemporalFXSubsystem.h"
#include "TemporalAimComponent.h"
#include "InverseStepComponent.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"

UObjectInversionComponent::UObjectInversionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UObjectInversionComponent::BeginPlay()
{
	Super::BeginPlay();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(GetOwner(), TEXT("TemporalEnergy"));
	Aim = TemporalFX::FindOrAddComponent<UTemporalAimComponent>(GetOwner(), TEXT("TemporalAim"));
}

void UObjectInversionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndInversion();
	Super::EndPlay(EndPlayReason);
}

bool UObjectInversionComponent::BeginInversion()
{
	if (bInverting)
	{
		return true;
	}

	// Siempre buscamos de nuevo al presionar: la selección es del frame anterior y la cámara pudo girar
	// (el tour "caos" lo encontró: R mirando al cielo gastaba energía en el objeto que estaba resaltado antes).
	// Lo que no tiene Registro (un maniquí, una torreta) no se puede invertir: la barra parpadea.
	AActor* Target = Aim ? Aim->FindTarget() : nullptr;
	return StartInversionOn(Target ? Target->FindComponentByClass<UTemporalRecorderComponent>() : nullptr);
}

bool UObjectInversionComponent::StartInversionOn(UTemporalRecorderComponent* Target)
{
	if (!Target || !Energy || !Energy->CanAfford(MinToStart))
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}

	// F y R no van juntos: empezar a invertir un objeto suelta el Paso Inverso.
	if (UInverseStepComponent* Inverse = GetOwner()->FindComponentByClass<UInverseStepComponent>())
	{
		Inverse->EndInverseStep();
	}

	if (Aim)
	{
		Aim->LockTarget(Target->GetOwner());
	}
	TSet<UTemporalRecorderComponent*> Group;
	Target->GatherGroup(Group);
	ActiveGroup.Reset();
	for (UTemporalRecorderComponent* Member : Group)
	{
		Member->StartRewind();
		ActiveGroup.Add(Member);
	}

	bInverting = true;
	Energy->SetActivePower(TEXT("Inversion"));
	TemporalFX::Burst(this, Target->GetOwner()->GetActorLocation(), FLinearColor(1.f, 0.65f, 0.15f), 0.5f);
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Target->GetOwner()->GetActorLocation(), -0.025f, 0.5f, 0.25f);
	}
	return true;
}

bool UObjectInversionComponent::BeginInversionOn(UTemporalRecorderComponent* Target)
{
	if (!Target || bInverting)
	{
		return false;
	}
	return StartInversionOn(Target);
}

void UObjectInversionComponent::EndInversion()
{
	if (!bInverting)
	{
		return;
	}
	bInverting = false;
	if (Aim)
	{
		Aim->UnlockTarget();
	}

	for (const TWeakObjectPtr<UTemporalRecorderComponent>& Member : ActiveGroup)
	{
		if (Member.IsValid() && Member->IsRewinding())
		{
			Member->StopRewind();
		}
	}
	ActiveGroup.Reset();

	if (Energy && Energy->GetActivePower() == TEXT("Inversion"))
	{
		Energy->SetActivePower(NAME_None);
	}
}

void UObjectInversionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bInverting)
	{
		// Termina solo si se acaba la energía o si ya nadie del grupo se está rebobinando
		// (llegó al inicio de su grabación, o la bala volvió al cañón y desapareció).
		bool bAnyRewinding = false;
		for (const TWeakObjectPtr<UTemporalRecorderComponent>& Member : ActiveGroup)
		{
			bAnyRewinding |= Member.IsValid() && Member->IsRewinding();
		}
		const bool bHasEnergy = Energy && Energy->Drain(CostPerSecond, TemporalFX::RealDelta());
		if (!bHasEnergy || !bAnyRewinding)
		{
			EndInversion();
		}
	}
}
