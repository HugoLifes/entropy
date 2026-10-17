// ENTROPÍA · Dash

#include "TemporalDashComponent.h"
#include "InverseStepComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalHealthComponent.h"
#include "TemporalTrailComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace EntropiaDash
{
	// Animación de la plantilla Platforming (no se modifica; solo se reproduce).
	const TCHAR* MontagePath = TEXT("/Game/Variant_Platforming/Anims/AM_Dash.AM_Dash");
}

UTemporalDashComponent::UTemporalDashComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTemporalDashComponent::BeginPlay()
{
	Super::BeginPlay();
	Trail = TemporalFX::FindOrAddComponent<UTemporalTrailComponent>(GetOwner(), TEXT("TemporalTrail"));
	DashMontage = LoadObject<UAnimMontage>(nullptr, EntropiaDash::MontagePath);

	// El movimiento del personaje corre DESPUÉS de este componente: así la velocidad del dash se usa en el mismo frame.
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
	}
}

void UTemporalDashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bDashing)
	{
		EndDash();
	}
	Super::EndPlay(EndPlayReason);
}

FVector UTemporalDashComponent::ComputeDashDirection(const FVector& MoveInput, const FVector& Facing)
{
	const FVector Move2D(MoveInput.X, MoveInput.Y, 0.f);
	if (Move2D.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		return Move2D.GetSafeNormal();
	}
	const FVector Facing2D(Facing.X, Facing.Y, 0.f);
	return Facing2D.SizeSquared() > KINDA_SMALL_NUMBER ? Facing2D.GetSafeNormal() : FVector::ForwardVector;
}

bool UTemporalDashComponent::CanDash(float SinceLastDash, float InCooldown, bool bFalling, bool bAirDashUsed)
{
	return SinceLastDash >= InCooldown && !(bFalling && bAirDashUsed);
}

bool UTemporalDashComponent::TryDash()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || bDashing)
	{
		return false;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	if (!Move || (Inverse && Inverse->IsInverseActive()))
	{
		return false;
	}
	const bool bFalling = Move->IsFalling();
	if (!CanDash(SinceLastDash, Cooldown, bFalling, bAirDashUsed))
	{
		return false;
	}

	DashDirection = ComputeDashDirection(Character->GetLastMovementInputVector(), Character->GetActorForwardVector());
	bAirDashUsed |= bFalling;

	// Sin fricción ni gravedad durante el dash: la velocidad la fija TickComponent cada frame.
	SavedWalkSpeed = Move->MaxWalkSpeed;
	SavedGroundFriction = Move->GroundFriction;
	SavedBrakingWalking = Move->BrakingDecelerationWalking;
	SavedBrakingFalling = Move->BrakingDecelerationFalling;
	SavedGravityScale = Move->GravityScale;
	Move->MaxWalkSpeed = DashSpeed;
	Move->GroundFriction = 0.f;
	Move->BrakingDecelerationWalking = 0.f;
	Move->BrakingDecelerationFalling = 0.f;
	Move->GravityScale = 0.f;
	Move->Velocity = DashDirection * DashSpeed;

	if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
	{
		Health->SetInvulnerable(InvulnerableSeconds);
	}

	bDashing = true;
	DashTimer = 0.f;
	SinceLastDash = 0.f;
	TrailTimer = 0.f;
	EmitTrail();

	if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (DashMontage)
		{
			// La animación no debe mover al personaje (el dash va hacia donde tú dices, no hacia donde mira).
			Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
			Anim->Montage_Play(DashMontage, 1.3f);
		}
	}
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Character->GetActorLocation(), 0.02f, 0.35f, 0.18f);
	}
	return true;
}

void UTemporalDashComponent::EndDash()
{
	bDashing = false;
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->MaxWalkSpeed = SavedWalkSpeed;
	Move->GroundFriction = SavedGroundFriction;
	Move->BrakingDecelerationWalking = SavedBrakingWalking;
	Move->BrakingDecelerationFalling = SavedBrakingFalling;
	Move->GravityScale = SavedGravityScale;
	// Sales del dash a velocidad de caminar en la misma dirección (sin frenón ni patinada).
	Move->Velocity = FVector(DashDirection.X * SavedWalkSpeed, DashDirection.Y * SavedWalkSpeed, Move->Velocity.Z);

	if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		if (DashMontage && Anim->Montage_IsPlaying(DashMontage))
		{
			Anim->Montage_Stop(0.25f, DashMontage);
		}
	}
}

void UTemporalDashComponent::EmitTrail()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Trail || !Body || !Body->GetSkeletalMeshAsset())
	{
		return;
	}
	Trail->Emit(Body->GetSkeletalMeshAsset(), Body->GetBoneSpaceTransforms(), Body->GetComponentTransform(), TrailColor);
}

void UTemporalDashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();

	// Tiempo real: en los tiempos lentos globales el jugador va a velocidad normal, y el dash también.
	const float RealDelta = TemporalFX::RealDelta();
	SinceLastDash += RealDelta;
	if (!bDashing && !Move->IsFalling())
	{
		bAirDashUsed = false;
	}
	if (!bDashing)
	{
		return;
	}

	// Si empieza el Paso Inverso a medio dash, manda el Paso Inverso.
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	DashTimer += RealDelta;
	if (DashTimer >= DashSeconds || (Inverse && Inverse->IsInverseActive()))
	{
		EndDash();
		return;
	}

	Move->Velocity = DashDirection * DashSpeed;
	TrailTimer += RealDelta;
	if (TrailInterval > 0.f && TrailTimer >= TrailInterval)
	{
		TrailTimer = 0.f;
		EmitTrail();
	}
}
