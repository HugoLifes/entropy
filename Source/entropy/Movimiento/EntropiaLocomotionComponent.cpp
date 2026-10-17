// ENTROPÍA · Locomoción

#include "EntropiaLocomotionComponent.h"
#include "EntropiaAnimationSet.h"
#include "TemporalAimComponent.h"
#include "TemporalDashComponent.h"

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

void UEntropiaLocomotionComponent::ApplyCrouchPose()
{
	ACharacter* Character = GetCharacter();
	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (!Character || !Set || !Character->GetMesh())
	{
		return;
	}
	UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
	const bool bPosePlaying = EntropiaAnim::IsPlaying(Character, Set->CrouchPose);
	if (IsCrouched())
	{
		// Agachado: la pose se pone si no hay otra animación encima (un golpe, un salto de parkour).
		if (!bPosePlaying && Anim && !Anim->IsAnyMontagePlaying())
		{
			EntropiaAnim::Play(Character, Set->CrouchPose);
		}
		bCrouchPosePlaying = true;
	}
	else if (bCrouchPosePlaying)
	{
		bCrouchPosePlaying = false;
		if (bPosePlaying)
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

	// Durante el dash manda el dash (él devuelve la velocidad al terminar).
	if (!(Dash && Dash->IsDashing()))
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		const float AimAlpha = Aim ? Aim->GetAimAlpha() : 0.f;
		Move->MaxWalkSpeed = ComputeWalkSpeed(bWantsSprint, false, AimAlpha, WalkSpeed, SprintSpeed, CrouchSpeed, AimSpeed);
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	ApplyCrouchPose();
}
