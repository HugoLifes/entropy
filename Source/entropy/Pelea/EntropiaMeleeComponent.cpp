// ENTROPÍA · Golpes

#include "EntropiaMeleeComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaParkourComponent.h"
#include "InverseStepComponent.h"
#include "TakedownComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalMannequin.h"
#include "TemporalSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"

int32 FEntropiaCombo::Next(float Now)
{
	const float Gap = Now - LastClick;
	if (Gap < MinGap)
	{
		return 0;
	}
	Index = (Gap <= Window && Index > 0 && Index < MaxHits) ? Index + 1 : 1;
	LastClick = Now;
	return Index;
}

UEntropiaMeleeComponent::UEntropiaMeleeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

ACharacter* UEntropiaMeleeComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

ATemporalMannequin* UEntropiaMeleeComponent::FindTarget(float MaxDistance, float HalfConeDegrees) const
{
	const ACharacter* Character = GetCharacter();
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	if (!Character || !Subsystem)
	{
		return nullptr;
	}
	const FVector From = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfConeDegrees));
	ATemporalMannequin* Best = nullptr;
	float BestDistance = MaxDistance;
	for (AActor* Actor : Subsystem->GetTimeTargets())
	{
		ATemporalMannequin* Mannequin = Cast<ATemporalMannequin>(Actor);
		if (!Mannequin || Mannequin->IsKnockedDown())
		{
			continue;
		}
		FVector To = Mannequin->GetActorLocation() - From;
		To.Z = 0.f;
		const float Distance = To.Size();
		if (Distance < BestDistance && FVector::DotProduct(To.GetSafeNormal(), Forward) >= MinDot)
		{
			Best = Mannequin;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool UEntropiaMeleeComponent::TryAttack()
{
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	const UEntropiaParkourComponent* Parkour = Character->FindComponentByClass<UEntropiaParkourComponent>();
	const UTakedownComponent* Takedown = Character->FindComponentByClass<UTakedownComponent>();
	if ((Inverse && Inverse->IsInverseActive()) || (Parkour && Parkour->IsBusy()) || (Takedown && Takedown->IsBusy()))
	{
		return false;
	}
	const int32 Index = Combo.Next(Clock);
	if (Index == 0)
	{
		return false;
	}

	// Te giras hacia el maniquí más cercano enfrente (un cono generoso) para no pegarle al aire.
	if (const ATemporalMannequin* Target = FindTarget(Reach + 120.f, 75.f))
	{
		const FVector To = Target->GetActorLocation() - Character->GetActorLocation();
		Character->SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}

	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		const FEntropiaAnimSlot& Slot = Index == 1 ? Set->Punch1 : (Index == 2 ? Set->Punch2 : Set->Punch3);
		EntropiaAnim::Play(Character, Slot);
	}
	PendingHit = HitDelay;
	PendingIndex = Index;
	LastIndex = Index;
	return true;
}

void UEntropiaMeleeComponent::LandHit()
{
	ACharacter* Character = GetCharacter();
	ATemporalMannequin* Target = FindTarget(Reach, ConeDegrees * 0.5f);
	if (!Character || !Target)
	{
		return;
	}
	FVector Direction = Target->GetActorLocation() - Character->GetActorLocation();
	Direction.Z = 0.f;
	Direction = Direction.GetSafeNormal();
	const bool bFinisher = PendingIndex >= Combo.MaxHits;
	Target->ReceiveHit(bFinisher ? (Direction + FVector(0.f, 0.f, 0.3f)).GetSafeNormal() : Direction, bFinisher);
	++HitsLanded;
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Target->GetActorLocation() + FVector(0.f, 0.f, 50.f), bFinisher ? 0.035f : 0.015f, 0.3f, bFinisher ? 0.25f : 0.12f);
	}
}

void UEntropiaMeleeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Tiempo real: los golpes van igual con la Sobremarcha (tú vas a velocidad normal).
	const float RealDelta = TemporalFX::RealDelta();
	Clock += RealDelta;
	if (PendingHit >= 0.f)
	{
		PendingHit -= RealDelta;
		if (PendingHit < 0.f)
		{
			LandHit();
		}
	}
}
