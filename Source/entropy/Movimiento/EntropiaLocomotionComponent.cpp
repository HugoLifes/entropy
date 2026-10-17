// ENTROPÍA · Locomoción

#include "EntropiaLocomotionComponent.h"
#include "EntropiaAnimationSet.h"
#include "TemporalAimComponent.h"
#include "TemporalDashComponent.h"
#include "TemporalHealthComponent.h"
#include "InverseStepComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

UEntropiaLocomotionComponent::UEntropiaLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

float UEntropiaLocomotionComponent::ComputeWalkSpeed(bool bSprint, bool bCrouch, float AimAlpha, float InWalk, float InSprint, float InCrouch, float InAim)
{
	if (bCrouch)
	{
		return InCrouch;
	}
	return FMath::Lerp(bSprint ? InSprint : InWalk, InAim, FMath::Clamp(AimAlpha, 0.f, 1.f));
}

ACharacter* UEntropiaLocomotionComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UEntropiaLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(CrouchedHalfHeight);
	Move->RotationRate = FRotator(0.f, TurnRate, 0.f);
	Move->MaxAcceleration = Acceleration;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;

	// La cámara sigue con un poco de retraso: se siente el peso al arrancar y al frenar.
	if (USpringArmComponent* Boom = Character->FindComponentByClass<USpringArmComponent>())
	{
		Boom->bEnableCameraLag = true;
		Boom->CameraLagSpeed = 14.f;
	}

	// La velocidad la escribe este componente: que el movimiento la lea después en el mismo frame.
	Move->AddTickPrerequisiteComponent(this);

	Character->LandedDelegate.AddDynamic(this, &UEntropiaLocomotionComponent::HandleLanded);
	LastLocation = Character->GetActorLocation();
	ApexZ = LastLocation.Z;
}

void UEntropiaLocomotionComponent::ResetFall()
{
	if (const ACharacter* Character = GetCharacter())
	{
		ApexZ = Character->GetActorLocation().Z;
		LastLocation = Character->GetActorLocation();
	}
}

void UEntropiaLocomotionComponent::TrackFall()
{
	ACharacter* Character = GetCharacter();
	const FVector Location = Character->GetActorLocation();
	// Un salto enorme en un frame es un teletransporte (salas, reaparecer): no cuenta como caída.
	if (FVector::DistSquared(Location, LastLocation) > FMath::Square(500.f))
	{
		ApexZ = Location.Z;
	}
	LastLocation = Location;

	const bool bFalling = Character->GetCharacterMovement()->IsFalling();
	if (bFalling)
	{
		ApexZ = bWasFalling ? FMath::Max(ApexZ, Location.Z) : Location.Z;
	}
	bWasFalling = bFalling;
}

void UEntropiaLocomotionComponent::HandleLanded(const FHitResult& Hit)
{
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	if (Inverse && Inverse->IsInverseActive())
	{
		return;
	}
	const float Height = FMath::Max(0.f, ApexZ - Character->GetActorLocation().Z);
	const FVector Velocity = Character->GetCharacterMovement()->Velocity;
	float Damage = 0.f;
	const EEntropiaLanding Kind = EntropiaFalls::Decide(Height, Velocity.Size2D(), Damage);
	LastLanding.Kind = Kind;
	LastLanding.Height = Height;
	LastLanding.Damage = Damage;
	++LastLanding.Count;
	ApexZ = Character->GetActorLocation().Z;

	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (Kind == EEntropiaLanding::Roll)
	{
		StartRoll(Velocity.GetSafeNormal2D());
	}
	else if (Kind == EEntropiaLanding::Hard)
	{
		HardTimer = HardLandingSeconds;
		if (Set)
		{
			EntropiaAnim::Play(Character, Set->HardLanding);
		}
	}
	if (Damage > 0.f)
	{
		if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
		{
			Health->ApplyDamage(Damage);
		}
	}
}

void UEntropiaLocomotionComponent::StartRoll(const FVector& Direction)
{
	ACharacter* Character = GetCharacter();
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body)
	{
		return;
	}
	RollDirection = Direction.IsNearlyZero() ? Character->GetActorForwardVector() : Direction;
	Character->SetActorRotation(FRotator(0.f, RollDirection.Rotation().Yaw, 0.f));
	MeshBaseLocation = Body->GetRelativeLocation();
	MeshBaseRotation = Body->GetRelativeRotation();
	RollTimer = RollSeconds;
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(Character, Set->Roll);
	}
}

void UEntropiaLocomotionComponent::UpdateRoll(float DeltaTime)
{
	ACharacter* Character = GetCharacter();
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body)
	{
		RollTimer = 0.f;
		return;
	}
	RollTimer -= DeltaTime;
	if (RollTimer <= 0.f)
	{
		EndRoll();
		return;
	}
	// Rodar de verdad: el cuerpo (encogido) da una vuelta completa hacia el frente alrededor del centro de la cápsula.
	const float Alpha = 1.f - RollTimer / RollSeconds;
	const float Eased = Alpha * Alpha * (3.f - 2.f * Alpha);
	const FQuat Spin = FRotator(-360.f * Eased, 0.f, 0.f).Quaternion();
	Body->SetRelativeLocationAndRotation(Spin.RotateVector(MeshBaseLocation), Spin * MeshBaseRotation.Quaternion());
	Character->GetCharacterMovement()->Velocity.X = RollDirection.X * RollSpeed;
	Character->GetCharacterMovement()->Velocity.Y = RollDirection.Y * RollSpeed;
}

void UEntropiaLocomotionComponent::EndRoll()
{
	RollTimer = 0.f;
	ACharacter* Character = GetCharacter();
	if (USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr)
	{
		Body->SetRelativeLocationAndRotation(MeshBaseLocation, MeshBaseRotation);
	}
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get(); Set && Character && EntropiaAnim::IsPlaying(Character, Set->Roll))
	{
		EntropiaAnim::Stop(Character, 0.15f);
	}
}

void UEntropiaLocomotionComponent::SetSprint(bool bSprint)
{
	bWantsSprint = bSprint;
	if (bSprint && IsCrouched())
	{
		SetCrouched(false);
	}
}

bool UEntropiaLocomotionComponent::IsCrouched() const
{
	const ACharacter* Character = GetCharacter();
	return Character && Character->bIsCrouched;
}

void UEntropiaLocomotionComponent::ToggleCrouch()
{
	SetCrouched(!IsCrouched());
}

void UEntropiaLocomotionComponent::SetCrouched(bool bCrouch)
{
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	if (bCrouch)
	{
		bWantsSprint = false;
		Character->Crouch();
	}
	else
	{
		Character->UnCrouch();
	}
}

void UEntropiaLocomotionComponent::SetCrouchPoseOverride(const FEntropiaAnimSlot* Pose)
{
	if (Pose)
	{
		CrouchPoseOverride = *Pose;
	}
	else
	{
		CrouchPoseOverride.Reset();
	}
}

void UEntropiaLocomotionComponent::ApplyCrouchPose()
{
	ACharacter* Character = GetCharacter();
	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (!Character || !Set || !Character->GetMesh())
	{
		return;
	}
	UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
	const FEntropiaAnimSlot& Desired = CrouchPoseOverride.IsSet() ? CrouchPoseOverride.GetValue() : Set->CrouchPose;
	if (IsCrouched())
	{
		// Agachado: la pose se pone si no hay otra animación encima (un golpe, un salto de parkour); si lo que hay
		// encima es la otra pose de agachado (la normal en vez de la de cobertura), se cambia directo.
		const bool bDesiredPlaying = EntropiaAnim::IsPlaying(Character, Desired);
		const bool bOtherPosePlaying = bCrouchPosePlaying && EntropiaAnim::IsPlaying(Character, PlayingCrouchPose);
		if (!bDesiredPlaying && Anim && (!Anim->IsAnyMontagePlaying() || bOtherPosePlaying))
		{
			EntropiaAnim::Play(Character, Desired);
			PlayingCrouchPose = Desired;
		}
		bCrouchPosePlaying = true;
	}
	else if (bCrouchPosePlaying)
	{
		bCrouchPosePlaying = false;
		if (EntropiaAnim::IsPlaying(Character, PlayingCrouchPose))
		{
			EntropiaAnim::Stop(Character, 0.2f);
		}
	}
}

void UEntropiaLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	if (!Aim)
	{
		Aim = Character->FindComponentByClass<UTemporalAimComponent>();
	}
	if (!Dash)
	{
		Dash = Character->FindComponentByClass<UTemporalDashComponent>();
	}

	if (!TourInput.IsNearlyZero())
	{
		Character->AddMovementInput(TourInput.GetSafeNormal2D());
	}

	TrackFall();
	if (RollTimer > 0.f)
	{
		UpdateRoll(DeltaTime);
	}
	HardTimer = FMath::Max(0.f, HardTimer - DeltaTime);

	// Durante el dash manda el dash (él devuelve la velocidad al terminar).
	if (!(Dash && Dash->IsDashing()))
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		const float AimAlpha = Aim ? Aim->GetAimAlpha() : 0.f;
		Move->MaxWalkSpeed = ComputeWalkSpeed(bWantsSprint, false, AimAlpha, WalkSpeed, SprintSpeed, CrouchSpeed, AimSpeed);
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
		// Aterrizaje duro: un momento sin poder moverte.
		if (HardTimer > 0.f)
		{
			Move->MaxWalkSpeed = 0.f;
		}
	}

	ApplyCrouchPose();
}
