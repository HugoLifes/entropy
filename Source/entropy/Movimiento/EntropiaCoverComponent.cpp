// ENTROPÍA · Cobertura

#include "EntropiaCoverComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaParkourComponent.h"
#include "InverseStepComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalDashComponent.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace EntropiaCover
{
	EEntropiaCover KindForHeight(float Height)
	{
		if (Height >= HighMinHeight)
		{
			return EEntropiaCover::High;
		}
		return Height >= LowMinHeight ? EEntropiaCover::Low : EEntropiaCover::None;
	}
}

UEntropiaCoverComponent::UEntropiaCoverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

ACharacter* UEntropiaCoverComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

bool UEntropiaCoverComponent::WallAt(const FVector& Location, float* OutHeight) const
{
	const ACharacter* Character = GetCharacter();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float FeetZ = Location.Z - Capsule->GetScaledCapsuleHalfHeight();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaCoverWall), false, Character);
	const FVector Knee(Location.X, Location.Y, FeetZ + 50.f);
	FHitResult Face;
	if (!GetWorld()->LineTraceSingleByChannel(Face, Knee, Knee - CoverNormal * (Radius + 40.f), ECC_Visibility, Params)
		|| (Face.GetActor() && Face.GetActor()->IsA<APawn>()))
	{
		return false;
	}
	if (OutHeight)
	{
		const FVector Inside = Face.ImpactPoint - CoverNormal * 10.f;
		FHitResult Top;
		const bool bTop = GetWorld()->LineTraceSingleByChannel(Top, FVector(Inside.X, Inside.Y, FeetZ + 400.f), FVector(Inside.X, Inside.Y, FeetZ), ECC_Visibility, Params);
		*OutHeight = (!bTop || Top.bStartPenetrating) ? 999.f : Top.ImpactPoint.Z - FeetZ;
	}
	return true;
}

bool UEntropiaCoverComponent::TryEnter()
{
	ACharacter* Character = GetCharacter();
	if (!Character || IsInCover())
	{
		return false;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const UEntropiaParkourComponent* Parkour = Character->FindComponentByClass<UEntropiaParkourComponent>();
	if (!Move || !Move->IsMovingOnGround() || (Parkour && Parkour->IsBusy()))
	{
		return false;
	}

	// La pared más cercana: primero hacia donde empujas el stick, luego hacia donde miras, luego alrededor.
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const FVector Center = Character->GetActorLocation();
	const float FeetZ = Center.Z - Capsule->GetScaledCapsuleHalfHeight();
	FVector Input = Character->GetLastMovementInputVector();
	Input.Z = 0.f;
	TArray<FVector> Directions;
	if (!Input.IsNearlyZero())
	{
		Directions.Add(Input.GetSafeNormal());
	}
	const FVector Forward = Character->GetActorForwardVector();
	const FVector Right = Character->GetActorRightVector();
	Directions.Append({ Forward, Right, -Right, -Forward });

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaCoverFind), false, Character);
	const FVector Knee(Center.X, Center.Y, FeetZ + 50.f);
	FHitResult Best;
	bool bFound = false;
	for (const FVector& Dir : Directions)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Knee, Knee + Dir * (Radius + EntropiaCover::MaxDistance), ECC_Visibility, Params)
			&& !(Hit.GetActor() && Hit.GetActor()->IsA<APawn>()) && FMath::Abs(Hit.ImpactNormal.Z) < 0.3f
			&& (!bFound || Hit.Distance < Best.Distance))
		{
			Best = Hit;
			bFound = true;
		}
	}
	if (!bFound)
	{
		return false;
	}
	CoverNormal = Best.ImpactNormal;
	CoverNormal.Z = 0.f;
	CoverNormal = CoverNormal.GetSafeNormal();

	float Height = 0.f;
	FVector Snap = Best.ImpactPoint + CoverNormal * (Radius + 6.f);
	Snap.Z = Center.Z;
	if (!WallAt(Snap, &Height))
	{
		return false;
	}
	const EEntropiaCover NewKind = EntropiaCover::KindForHeight(Height);
	if (NewKind == EEntropiaCover::None)
	{
		return false;
	}

	Kind = NewKind;
	Character->SetActorLocationAndRotation(Snap, FRotator(0.f, (-CoverNormal).Rotation().Yaw, 0.f), true);
	LastValidLocation = Character->GetActorLocation();
	// Moverse solo a lo largo de la pared.
	Move->SetPlaneConstraintEnabled(true);
	Move->SetPlaneConstraintNormal(CoverNormal);
	Move->SetPlaneConstraintOrigin(LastValidLocation);
	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (UEntropiaLocomotionComponent* Locomotion = Character->FindComponentByClass<UEntropiaLocomotionComponent>())
	{
		Locomotion->SetSprint(false);
		// Baja: agachado con la pose de cobertura (hueco CoverLow de DA_Animaciones).
		Locomotion->SetCrouchPoseOverride(Kind == EEntropiaCover::Low && Set ? &Set->CoverLow : nullptr);
		Locomotion->SetCrouched(Kind == EEntropiaCover::Low);
	}
	if (Kind == EEntropiaCover::High && Set && !Set->CoverHigh.Anim.IsNull())
	{
		EntropiaAnim::Play(Character, Set->CoverHigh);
	}
	CornerSide = 0;
	bPeeking = false;
	PushAwayTime = 0.f;
	return true;
}

void UEntropiaCoverComponent::Exit()
{
	if (!IsInCover())
	{
		return;
	}
	ACharacter* Character = GetCharacter();
	if (Character)
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		Move->SetPlaneConstraintEnabled(false);
		if (bPeeking && Kind == EEntropiaCover::High)
		{
			Character->SetActorLocation(PeekBase, true);
		}
		if (UTemporalAimComponent* Aim = Character->FindComponentByClass<UTemporalAimComponent>())
		{
			Aim->SetShoulderSide(1.f);
		}
		if (UEntropiaLocomotionComponent* Locomotion = Character->FindComponentByClass<UEntropiaLocomotionComponent>())
		{
			Locomotion->SetCrouchPoseOverride(nullptr);
			if (Kind == EEntropiaCover::Low)
			{
				Locomotion->SetCrouched(false);
			}
		}
		if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get(); Set && EntropiaAnim::IsPlaying(Character, Set->CoverHigh))
		{
			EntropiaAnim::Stop(Character, 0.2f);
		}
	}
	Kind = EEntropiaCover::None;
	CornerSide = 0;
	bPeeking = false;
}

void UEntropiaCoverComponent::UpdatePeek(bool bAiming)
{
	ACharacter* Character = GetCharacter();
	UEntropiaLocomotionComponent* Locomotion = Character->FindComponentByClass<UEntropiaLocomotionComponent>();
	if (bAiming == bPeeking)
	{
		return;
	}
	if (Kind == EEntropiaCover::Low)
	{
		// Baja: te levantas por encima de la barda mientras apuntas.
		bPeeking = bAiming;
		if (Locomotion)
		{
			Locomotion->SetCrouched(!bAiming);
		}
		return;
	}
	// Alta: solo te asomas si estás en una esquina; sales de lado y regresas al soltar.
	UTemporalAimComponent* Aim = Character->FindComponentByClass<UTemporalAimComponent>();
	if (bAiming && CornerSide != 0)
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, -CoverNormal).GetSafeNormal();
		// La cámara pasa al hombro del lado de la esquina: así ves lo que hay detrás de ella.
		if (Aim)
		{
			Aim->SetShoulderSide(static_cast<float>(CornerSide));
		}
		PeekBase = Character->GetActorLocation();
		if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get(); Set && EntropiaAnim::IsPlaying(Character, Set->CoverHigh))
		{
			EntropiaAnim::Stop(Character, 0.15f);
		}
		Character->GetCharacterMovement()->SetPlaneConstraintEnabled(false);
		Character->SetActorLocation(PeekBase + Right * CornerSide * PeekOut, true);
		bPeeking = true;
	}
	else if (!bAiming)
	{
		Character->SetActorLocation(PeekBase, true);
		Character->GetCharacterMovement()->SetPlaneConstraintEnabled(true);
		bPeeking = false;
		if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get(); Set && !Set->CoverHigh.Anim.IsNull())
		{
			EntropiaAnim::Play(Character, Set->CoverHigh);
		}
		if (Aim)
		{
			Aim->SetShoulderSide(1.f);
		}
	}
}

void UEntropiaCoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsInCover())
	{
		return;
	}
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		Kind = EEntropiaCover::None;
		return;
	}

	// Cosas que te sacan de la cobertura.
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	const UEntropiaParkourComponent* Parkour = Character->FindComponentByClass<UEntropiaParkourComponent>();
	const UTemporalDashComponent* Dash = Character->FindComponentByClass<UTemporalDashComponent>();
	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if ((Inverse && Inverse->IsInverseActive()) || (Parkour && Parkour->IsBusy()) || (Dash && Dash->IsDashing()) || !Move->IsMovingOnGround())
	{
		Exit();
		return;
	}

	// Algo te movió de golpe (una prensa, una zona de caída): sales de la cobertura sin regresarte a la pared.
	const FVector Anchor = (bPeeking && Kind == EEntropiaCover::High) ? PeekBase : LastValidLocation;
	if (FVector::Dist2D(Character->GetActorLocation(), Anchor) > 200.f)
	{
		bPeeking = false;
		Exit();
		return;
	}

	const UTemporalAimComponent* Aim = Character->FindComponentByClass<UTemporalAimComponent>();
	const bool bAiming = Aim && Aim->IsAiming();
	if (bPeeking && Kind == EEntropiaCover::High)
	{
		// Asomado de lado: quieto hasta soltar el clic derecho.
		UpdatePeek(bAiming);
		return;
	}

	// ¿Sigue la pared detrás? Si te pasaste de la orilla, regresas; si la pared ya no está (se rompió), sales.
	const FVector Here = Character->GetActorLocation();
	if (!WallAt(Here))
	{
		if (!WallAt(LastValidLocation))
		{
			Exit();
			return;
		}
		Character->SetActorLocation(LastValidLocation, false);
	}
	else
	{
		LastValidLocation = Here;
	}

	// ¿Estás en una esquina? (la pared se acaba un poco más allá)
	const FVector Right = FVector::CrossProduct(FVector::UpVector, -CoverNormal).GetSafeNormal();
	const float Reach = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + 15.f;
	CornerSide = !WallAt(LastValidLocation + Right * Reach) ? 1 : (!WallAt(LastValidLocation - Right * Reach) ? -1 : 0);

	// Empujar el stick lejos de la pared un momento = salir.
	FVector Input = Character->GetLastMovementInputVector();
	Input.Z = 0.f;
	PushAwayTime = (!Input.IsNearlyZero() && FVector::DotProduct(Input.GetSafeNormal(), CoverNormal) > 0.7f) ? PushAwayTime + DeltaTime : 0.f;
	if (PushAwayTime > 0.25f)
	{
		Exit();
		return;
	}

	UpdatePeek(bAiming);
}
