// ENTROPÍA · Dash

#include "TemporalDashComponent.h"
#include "InverseStepComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalHealthComponent.h"
#include "TemporalCombat.h"
#include "TemporalDilation.h"
#include "TemporalProjectile.h"
#include "TemporalTrailComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"
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

	// Tiempo de reflejo: imagen un poco desaturada y cálida con viñeta (distinta de la Sobremarcha y del Paso Inverso).
	ReflexPostProcess = NewObject<UPostProcessComponent>(GetOwner(), TEXT("ReflexPostProcess"));
	ReflexPostProcess->bUnbound = true;
	ReflexPostProcess->Priority = 3.f;
	ReflexPostProcess->BlendWeight = 0.f;
	FPostProcessSettings& PP = ReflexPostProcess->Settings;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.55f, 0.55f, 0.55f, 1.f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(1.08f, 1.f, 0.9f, 1.f);
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.8f;
	ReflexPostProcess->RegisterComponent();

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
	EndReflex();
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

	// Esquiva perfecta: se mide ANTES de moverte (¿la bala te iba a pegar donde estabas?).
	const bool bPerfect = IsPerfectDodge();

	if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
	{
		Health->SetInvulnerable(InvulnerableSeconds);
	}
	if (bPerfect)
	{
		StartReflex();
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

bool UTemporalDashComponent::IsPerfectDodge() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Capsule || DodgeWindow <= 0.f)
	{
		return false;
	}
	for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
	{
		if (!It->bHurtsPlayer)
		{
			continue;
		}
		const float Seconds = TemporalCombat::TimeToHitCapsule(It->GetActorLocation(), It->GetRealVelocity(),
			Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), DodgeWindow);
		if (Seconds >= 0.f)
		{
			return true;
		}
	}
	return false;
}

void UTemporalDashComponent::StartReflex()
{
	ReflexTimer = ReflexSeconds;
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->RequestGlobal(this, ReflexDilation);
	}
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(GetOwner()->GetActorLocation(), 0.05f, 0.9f, 0.9f);
	}
	TemporalFX::Burst(this, GetOwner()->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.6f), 0.8f);
}

void UTemporalDashComponent::EndReflex()
{
	const bool bWasActive = ReflexTimer > 0.f;
	ReflexTimer = 0.f;
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseGlobal(this);
	}
	if (bWasActive && GetOwner())
	{
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(GetOwner()->GetActorLocation(), -0.02f, 0.4f, 0.3f);
		}
	}
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

	if (ReflexTimer > 0.f)
	{
		ReflexTimer -= RealDelta;
		if (ReflexTimer <= 0.f)
		{
			EndReflex();
		}
	}
	ReflexBlend = FMath::FInterpTo(ReflexBlend, ReflexTimer > 0.f ? 1.f : 0.f, RealDelta, ReflexTimer > 0.f ? 14.f : 5.f);
	if (ReflexPostProcess)
	{
		ReflexPostProcess->BlendWeight = ReflexBlend;
	}
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
