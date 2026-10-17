// ENTROPÍA · Detener

#include "TimeStopComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TimeBubble.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UTimeStopComponent::UTimeStopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTimeStopComponent::BeginPlay()
{
	Super::BeginPlay();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(GetOwner(), TEXT("TemporalEnergy"));
	Aim = TemporalFX::FindOrAddComponent<UTemporalAimComponent>(GetOwner(), TEXT("TemporalAim"));
}

void UTimeStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAll();
	Super::EndPlay(EndPlayReason);
}

void UTimeStopComponent::ForgetFinishedShells()
{
	Shells.RemoveAll([](const TWeakObjectPtr<ATimeBubble>& Shell) { return !Shell.IsValid() || Shell->IsActorBeingDestroyed(); });
}

ATimeBubble* UTimeStopComponent::FindShell(const AActor* Target) const
{
	for (const TWeakObjectPtr<ATimeBubble>& Shell : Shells)
	{
		if (Shell.IsValid() && !Shell->IsActorBeingDestroyed() && Target && Shell->OnlyTarget.Get() == Target)
		{
			return Shell.Get();
		}
	}
	return nullptr;
}

bool UTimeStopComponent::IsStoppedByMe(const AActor* Target) const
{
	return FindShell(Target) != nullptr;
}

int32 UTimeStopComponent::NumStopped() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ATimeBubble>& Shell : Shells)
	{
		Count += (Shell.IsValid() && !Shell->IsActorBeingDestroyed()) ? 1 : 0;
	}
	return Count;
}

bool UTimeStopComponent::StopAimed()
{
	// Buscamos de nuevo al hacer clic: la selección es del frame anterior y la cámara pudo girar.
	AActor* Target = Aim ? Aim->FindTarget() : nullptr;
	if (!Target)
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}
	if (IsStoppedByMe(Target))
	{
		ReleaseActor(Target);
		return true;
	}
	return StopActor(Target);
}

bool UTimeStopComponent::StopActor(AActor* Target)
{
	ForgetFinishedShells();
	if (!IsValid(Target) || Target == GetOwner())
	{
		return false;
	}
	if (IsStoppedByMe(Target))
	{
		return true;
	}
	if (Energy && !Energy->Spend(Cost))
	{
		Energy->Deny();
		return false;
	}

	// La burbujita envuelve al objetivo: chica para una bala, más grande para un maniquí.
	FVector Center, Extent;
	Target->GetActorBounds(false, Center, Extent);
	const float Radius = FMath::Clamp(Extent.Size() * 1.1f, 25.f, 250.f);
	const FTransform Transform(FRotator::ZeroRotator, Center);
	ATimeBubble* Shell = GetWorld()->SpawnActorDeferred<ATimeBubble>(ATimeBubble::StaticClass(), Transform);
	if (!Shell)
	{
		return false;
	}
	Shell->Dilation = 0.f;
	Shell->LifeSeconds = StopSeconds;
	Shell->OnlyTarget = Target;
	Shell->bDrawField = false;
	Shell->VisualColor = ShellColor;
	Shell->BurstScale = 0.35f;
	Shell->Sphere->SetSphereRadius(Radius);
	UGameplayStatics::FinishSpawningActor(Shell, Transform);
	Shells.Add(Shell);

	// La cuarta suelta la más vieja.
	while (NumStopped() > MaxStopped)
	{
		for (int32 i = 0; i < Shells.Num(); ++i)
		{
			if (Shells[i].IsValid() && !Shells[i]->IsActorBeingDestroyed())
			{
				Shells[i]->Destroy();
				break;
			}
		}
		ForgetFinishedShells();
	}

	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Center, 0.025f, 0.4f, 0.15f);
	}
	return true;
}

void UTimeStopComponent::ReleaseActor(AActor* Target)
{
	if (ATimeBubble* Shell = FindShell(Target))
	{
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(Shell->GetActorLocation(), -0.015f, 0.3f, 0.12f);
		}
		Shell->Destroy();
	}
	ForgetFinishedShells();
}

void UTimeStopComponent::ReleaseAll()
{
	for (const TWeakObjectPtr<ATimeBubble>& Shell : Shells)
	{
		if (Shell.IsValid() && !Shell->IsActorBeingDestroyed())
		{
			Shell->Destroy();
		}
	}
	Shells.Reset();
}
