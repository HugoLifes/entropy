// ENTROPÍA · Remates

#include "TakedownComponent.h"
#include "InverseStepComponent.h"
#include "TemporalDashComponent.h"
#include "EntropiaParkourComponent.h"
#include "EntropiaAnimationSet.h"
#include "TemporalAimComponent.h"
#include "TemporalDilation.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalMannequin.h"
#include "TemporalSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace EntropiaTakedown
{
	// Golpe de la plantilla Combat (no se modifica; solo se reproduce).
	const TCHAR* MontagePath = TEXT("/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack");
	/** Segundos (reales) que dura un remate: no puedes hacer otro ni dash mientras tanto. */
	constexpr float BusySeconds = 0.55f;
}

UTakedownComponent::UTakedownComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTakedownComponent::BeginPlay()
{
	Super::BeginPlay();
	AttackMontage = LoadObject<UAnimMontage>(nullptr, EntropiaTakedown::MontagePath);
}

void UTakedownComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndSlow();
	Super::EndPlay(EndPlayReason);
}

EEntropiaTakedown UTakedownComponent::ChooseKind(bool bBehind, bool bFrozen, bool bSubdued)
{
	return (bBehind || bSubdued) ? EEntropiaTakedown::NeckBreak : EEntropiaTakedown::FocusStrike;
}

bool UTakedownComponent::CanTakedown(const FVector& Attacker, const FVector& Target, const FVector& TargetForward, bool bFrozen, float InMaxDistance, float InBehindAngle)
{
	const FVector ToAttacker(Attacker.X - Target.X, Attacker.Y - Target.Y, 0.f);
	if (ToAttacker.SizeSquared() > FMath::Square(InMaxDistance) || FMath::Abs(Attacker.Z - Target.Z) > 150.f)
	{
		return false;
	}
	if (bFrozen)
	{
		return true;
	}
	// A su espalda: el ángulo entre su frente y la dirección hacia ti es mayor que InBehindAngle.
	const FVector Forward2D = FVector(TargetForward.X, TargetForward.Y, 0.f).GetSafeNormal();
	const float Cos = FVector::DotProduct(Forward2D, ToAttacker.GetSafeNormal());
	return Cos < FMath::Cos(FMath::DegreesToRadians(InBehindAngle));
}

ATemporalMannequin* UTakedownComponent::FindTarget() const
{
	const AActor* Owner = GetOwner();
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
	if (!Owner || !Subsystem)
	{
		return nullptr;
	}

	ATemporalMannequin* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (AActor* Actor : Subsystem->GetTimeTargets())
	{
		ATemporalMannequin* Mannequin = Cast<ATemporalMannequin>(Actor);
		if (!Mannequin || Mannequin->IsKnockedDown() || Mannequin->HasPendingHit())
		{
			continue;
		}
		// Congelado o sometido: se puede de cualquier lado.
		const bool bFree = (Dilation && Dilation->IsFrozen(Mannequin)) || Mannequin->IsSubdued();
		if (!CanTakedown(Owner->GetActorLocation(), Mannequin->GetActorLocation(), Mannequin->GetActorForwardVector(), bFree, MaxDistance, BehindAngle))
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Owner->GetActorLocation(), Mannequin->GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = Mannequin;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool UTakedownComponent::TryTakedown()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UInverseStepComponent* Inverse = Character ? Character->FindComponentByClass<UInverseStepComponent>() : nullptr;
	const UEntropiaParkourComponent* Parkour = Character ? Character->FindComponentByClass<UEntropiaParkourComponent>() : nullptr;
	if (!Character || IsBusy() || (Inverse && Inverse->IsInverseActive()) || (Parkour && Parkour->IsBusy()))
	{
		return false;
	}
	ATemporalMannequin* Target = FindTarget();
	if (!Target)
	{
		return false;
	}

	// Alinearte: a su espalda si estás atrás; si está congelado de frente, frente a él.
	const bool bBehind = CanTakedown(Character->GetActorLocation(), Target->GetActorLocation(), Target->GetActorForwardVector(), false, MaxDistance, BehindAngle);
	const UTemporalDilationSubsystem* DilationNow = UTemporalDilationSubsystem::Get(this);
	const bool bFrozen = DilationNow && DilationNow->IsFrozen(Target);
	const bool bSubdued = Target->IsSubdued();
	CurrentKind = ChooseKind(bBehind, bFrozen, bSubdued);
	LastKind = CurrentKind;
	FVector Facing = bBehind ? Target->GetActorForwardVector() : (Target->GetActorLocation() - Character->GetActorLocation());
	Facing.Z = 0.f;
	Facing = Facing.GetSafeNormal();
	if (Facing.IsNearlyZero())
	{
		Facing = Character->GetActorForwardVector();
	}
	// Si venías en dash, el dash se corta aquí: si no, seguiría empujándote durante el golpe.
	if (UTemporalDashComponent* Dash = Character->FindComponentByClass<UTemporalDashComponent>())
	{
		Dash->CancelDash();
	}
	// Sometido: ya estás pegado a él (no te mueve el agarre).
	if (!bSubdued)
	{
		FVector Spot = Target->GetActorLocation() - Facing * AlignDistance;
		Spot.Z = Character->GetActorLocation().Z;
		Character->SetActorLocation(Spot, /*bSweep=*/true);
	}
	Character->SetActorRotation(Facing.Rotation());

	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (CurrentKind == EEntropiaTakedown::FocusStrike)
	{
		// Golpe con enfoque: cámara lenta breve (el árbitro te compensa a ti) y la cámara se acerca al golpe.
		if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
		{
			Dilation->RequestGlobal(this, SlowDilation);
		}
		SlowTimer = SlowSeconds;
		if (UTemporalAimComponent* Aim = Character->FindComponentByClass<UTemporalAimComponent>())
		{
			Aim->StartFocus(0.8f);
		}
		const float Played = Set ? EntropiaAnim::Play(Character, Set->FocusStrike) : 0.f;
		if (Played <= 0.f && AttackMontage)
		{
			if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
			{
				Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
				Anim->Montage_Play(AttackMontage, 1.4f);
			}
		}
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(Target->GetActorLocation(), 0.045f, 0.6f, 0.5f);
		}
	}
	else if (Set)
	{
		// Romper el cuello: rápido y en silencio (sin cámara lenta ni onda grande).
		EntropiaAnim::Play(Character, Set->NeckBreak);
	}

	Victim = Target;
	HitDirection = (Facing + FVector(0.f, 0.f, 0.35f)).GetSafeNormal();
	BusyTimer = EntropiaTakedown::BusySeconds;
	Elapsed = 0.f;
	bHitLanded = false;
	Available = nullptr;
	return true;
}

void UTakedownComponent::Cancel()
{
	EndSlow();
	BusyTimer = 0.f;
	Elapsed = 0.f;
	bHitLanded = true;
	Victim = nullptr;
	Available = nullptr;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && AttackMontage && Anim->Montage_IsPlaying(AttackMontage))
	{
		Anim->Montage_Stop(0.1f, AttackMontage);
	}
}

void UTakedownComponent::EndSlow()
{
	SlowTimer = 0.f;
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseGlobal(this);
	}
}

void UTakedownComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Tiempo real: el remate dura lo mismo aunque el mundo vaya lento.
	const float RealDelta = TemporalFX::RealDelta();
	if (SlowTimer > 0.f)
	{
		SlowTimer -= RealDelta;
		if (SlowTimer <= 0.f)
		{
			EndSlow();
		}
	}

	if (BusyTimer > 0.f)
	{
		BusyTimer -= RealDelta;
		Elapsed += RealDelta;
		if (!bHitLanded && Elapsed >= HitDelay)
		{
			bHitLanded = true;
			if (ATemporalMannequin* Target = Victim.Get())
			{
				if (CurrentKind == EEntropiaTakedown::NeckBreak)
				{
					Target->ReceiveNeckBreak(HitDirection);
				}
				else
				{
					Target->ReceiveTakedown(HitDirection, Impulse);
					TemporalFX::Burst(this, Target->GetActorLocation(), FLinearColor(1.f, 0.6f, 0.3f), 0.6f);
				}
			}
		}
		return;
	}

	Available = FindTarget();
	if (const ATemporalMannequin* Target = Available.Get())
	{
		const AActor* Owner = GetOwner();
		const bool bBehind = CanTakedown(Owner->GetActorLocation(), Target->GetActorLocation(), Target->GetActorForwardVector(), false, MaxDistance, BehindAngle);
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		AvailableKind = ChooseKind(bBehind, Dilation && Dilation->IsFrozen(Target), Target->IsSubdued());
	}
}
