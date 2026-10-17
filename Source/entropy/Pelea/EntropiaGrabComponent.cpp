// ENTROPÍA · Agarrar, desarmar y someter

#include "EntropiaGrabComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaParkourComponent.h"
#include "InverseStepComponent.h"
#include "TakedownComponent.h"
#include "TemporalArmedDummy.h"
#include "TemporalFX.h"
#include "TemporalMannequin.h"
#include "TemporalSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UEntropiaGrabComponent::UEntropiaGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

ACharacter* UEntropiaGrabComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

bool UEntropiaGrabComponent::CanGrab(const FVector& Attacker, const FVector& AttackerForward, const FVector& Target, bool bTargetStanding, float InMaxDistance)
{
	if (!bTargetStanding)
	{
		return false;
	}
	FVector To = Target - Attacker;
	To.Z = 0.f;
	if (To.SizeSquared() > FMath::Square(InMaxDistance))
	{
		return false;
	}
	const FVector Forward = FVector(AttackerForward.X, AttackerForward.Y, 0.f).GetSafeNormal();
	return FVector::DotProduct(Forward, To.GetSafeNormal()) >= FMath::Cos(FMath::DegreesToRadians(60.f));
}

ATemporalMannequin* UEntropiaGrabComponent::FindGrabbable() const
{
	const ACharacter* Character = GetCharacter();
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	if (!Character || !Subsystem)
	{
		return nullptr;
	}
	ATemporalMannequin* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (AActor* Actor : Subsystem->GetTimeTargets())
	{
		ATemporalMannequin* Mannequin = Cast<ATemporalMannequin>(Actor);
		// De pie: ni tirado, ni con un golpe guardado, ni ya sometido, ni desplomándose (aturdido sí se puede).
		const bool bStanding = Mannequin && !Mannequin->IsKnockedDown() && !Mannequin->HasPendingHit() && !Mannequin->IsSubdued()
			&& !Mannequin->IsCollapsing();
		if (!Mannequin || !CanGrab(Character->GetActorLocation(), Character->GetActorForwardVector(), Mannequin->GetActorLocation(), bStanding, MaxDistance))
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Character->GetActorLocation(), Mannequin->GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = Mannequin;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool UEntropiaGrabComponent::TryGrab()
{
	ACharacter* Character = GetCharacter();
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	if (!Character || !Subsystem || IsHolding())
	{
		return false;
	}
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	const UEntropiaParkourComponent* Parkour = Character->FindComponentByClass<UEntropiaParkourComponent>();
	const UTakedownComponent* Takedown = Character->FindComponentByClass<UTakedownComponent>();
	if ((Inverse && Inverse->IsInverseActive()) || (Parkour && Parkour->IsBusy()) || (Takedown && Takedown->IsBusy())
		|| !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}

	ATemporalMannequin* Best = FindGrabbable();
	if (!Best)
	{
		return false;
	}

	// Llave: le tomas el brazo; si tiene pistola, se la quitas. Queda de espaldas a ti, sometido.
	const FVector Direction = (Best->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	float PoseDelay = 0.f;
	if (ATemporalArmedDummy* Armed = Cast<ATemporalArmedDummy>(Best); Armed && Armed->IsArmed())
	{
		// Primero se ve cómo le quitas la pistola; después queda en la llave.
		PoseDelay = FMath::Min(Armed->Disarm(Direction), 0.6f);
	}
	Best->SetSubdued(true, PoseDelay);
	Held = Best;
	HoldTimer = HoldSeconds;
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->DisableMovement();
	PlaceHeld();
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(Character, Set->Grab);
	}
	TemporalFX::Burst(this, Best->GetActorLocation() + FVector(0.f, 0.f, 60.f), FLinearColor(1.f, 0.8f, 0.5f), 0.3f);
	return true;
}

void UEntropiaGrabComponent::PlaceHeld()
{
	ACharacter* Character = GetCharacter();
	ATemporalMannequin* Victim = Held.Get();
	if (!Character || !Victim)
	{
		return;
	}
	const FVector Forward = Character->GetActorForwardVector();
	FVector Spot = Character->GetActorLocation() + Forward * HoldDistance;
	Spot.Z = Victim->GetActorLocation().Z;
	Victim->SetActorLocationAndRotation(Spot, FRotator(0.f, Forward.Rotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void UEntropiaGrabComponent::Release(bool bShove)
{
	ATemporalMannequin* Victim = Held.Get();
	Held = nullptr;
	HoldTimer = 0.f;
	if (ACharacter* Character = GetCharacter())
	{
		if (Character->GetCharacterMovement()->MovementMode == MOVE_None)
		{
			const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
			if (!(Inverse && Inverse->IsInverseActive()))
			{
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		}
		if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get(); Set && EntropiaAnim::IsPlaying(Character, Set->Grab))
		{
			EntropiaAnim::Stop(Character, 0.2f);
		}
		if (Victim && Victim->IsSubdued())
		{
			Victim->SetSubdued(false);
			if (bShove)
			{
				Victim->ReceiveHit(Character->GetActorForwardVector(), false);
			}
		}
	}
}

void UEntropiaGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bHasGrabbable = !Held.IsValid() && FindGrabbable() != nullptr;
	if (!Held.IsValid())
	{
		if (HoldTimer > 0.f)
		{
			Release(false);
		}
		return;
	}
	ATemporalMannequin* Victim = Held.Get();
	ACharacter* Character = GetCharacter();
	const UInverseStepComponent* Inverse = Character ? Character->FindComponentByClass<UInverseStepComponent>() : nullptr;
	// Si ya no está sometido (le rompiste el cuello, lo tumbaron) o tú haces otra cosa, se suelta.
	if (!Character || !Victim->IsSubdued() || (Inverse && Inverse->IsInverseActive()))
	{
		Release(false);
		return;
	}
	HoldTimer -= TemporalFX::RealDelta();
	if (HoldTimer <= 0.f)
	{
		Release(true);
		return;
	}
	PlaceHeld();
}
