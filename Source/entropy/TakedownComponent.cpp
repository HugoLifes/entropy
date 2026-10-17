// ENTROPÍA · Remates

#include "TakedownComponent.h"
#include "InverseStepComponent.h"
#include "TemporalDashComponent.h"
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
		const bool bFrozen = Dilation && Dilation->IsFrozen(Mannequin);
		if (!CanTakedown(Owner->GetActorLocation(), Mannequin->GetActorLocation(), Mannequin->GetActorForwardVector(), bFrozen, MaxDistance, BehindAngle))
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
	if (!Character || IsBusy() || (Inverse && Inverse->IsInverseActive()))
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
	FVector Spot = Target->GetActorLocation() - Facing * AlignDistance;
	Spot.Z = Character->GetActorLocation().Z;
	Character->SetActorLocation(Spot, /*bSweep=*/true);
	Character->SetActorRotation(Facing.Rotation());

	// Cámara lenta breve (el árbitro te compensa a ti).
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->RequestGlobal(this, SlowDilation);
	}
	SlowTimer = SlowSeconds;

	if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AttackMontage)
		{
			// El golpe no debe mover al personaje (ya lo alineamos).
			Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
			Anim->Montage_Play(AttackMontage, 1.4f);
		}
	}
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Target->GetActorLocation(), 0.045f, 0.6f, 0.5f);
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
				Target->ReceiveTakedown(HitDirection, Impulse);
				TemporalFX::Burst(this, Target->GetActorLocation(), FLinearColor(1.f, 0.6f, 0.3f), 0.6f);
			}
		}
		return;
	}

	Available = FindTarget();
}
