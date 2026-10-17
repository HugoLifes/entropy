// ENTROPÍA · Maniquí temporal

#include "TemporalMannequin.h"
#include "TemporalDilation.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalSubsystem.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaGuns.h"
#include "TemporalProjectile.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodyInstance.h"

ATemporalMannequin::ATemporalMannequin()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	// Sin controlador igual cae al piso y se para (si no, flotaría donde aparece).
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetMesh()->SetRelativeLocationAndRotation(MeshRelativeLocation, MeshRelativeRotation);
}

void ATemporalMannequin::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	if (UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>())
	{
		Subsystem->RegisterTimeTarget(this);
	}
}

void ATemporalMannequin::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTemporalSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UTemporalSubsystem>() : nullptr)
	{
		Subsystem->UnregisterTimeTarget(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ATemporalMannequin::SetupFromPlayer(USkeletalMeshComponent* PlayerMesh)
{
	if (!PlayerMesh || !PlayerMesh->GetSkeletalMeshAsset())
	{
		return;
	}
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMesh(PlayerMesh->GetSkeletalMeshAsset());
	Body->SetAnimInstanceClass(PlayerMesh->GetAnimClass());
	MeshRelativeLocation = PlayerMesh->GetRelativeLocation();
	MeshRelativeRotation = PlayerMesh->GetRelativeRotation();
	Body->SetRelativeLocationAndRotation(MeshRelativeLocation, MeshRelativeRotation);

	// Los mismos materiales del jugador, oscurecidos y con los paneles en rojo apagado ("Paint Tint" del maniquí de Epic).
	for (int32 Slot = 0; Slot < PlayerMesh->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInterface* Source = PlayerMesh->GetMaterial(Slot))
		{
			UMaterialInstanceDynamic* Dark = UMaterialInstanceDynamic::Create(Source, this);
			Dark->SetVectorParameterValue(TEXT("Paint Tint"), FLinearColor(0.45f, 0.08f, 0.06f));
			Dark->SetVectorParameterValue(TEXT("Global BaseColor"), FLinearColor(0.3f, 0.32f, 0.38f));
			Body->SetMaterial(Slot, Dark);
		}
	}
}

void ATemporalMannequin::ReceiveTakedown(const FVector& Direction, float Impulse)
{
	if (bKnockedDown || bPendingHit)
	{
		return;
	}
	const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
	if (Dilation && Dilation->IsFrozen(this))
	{
		// Golpe guardado: sale cuando su tiempo vuelva a correr.
		bPendingHit = true;
		PendingDirection = Direction;
		PendingImpulse = Impulse;
		TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.6f, 0.92f, 1.f), 0.5f);
		return;
	}
	KnockDown(Direction, Impulse);
}

void ATemporalMannequin::ReceiveHit(const FVector& Direction, bool bKnockdown)
{
	if (bKnockedDown || bPendingHit)
	{
		return;
	}
	if (bKnockdown)
	{
		ReceiveTakedown(Direction, 700.f);
		return;
	}
	StaggerTime = StaggerSeconds;
	LaunchCharacter(FVector(Direction.X, Direction.Y, 0.f).GetSafeNormal() * 250.f, true, false);
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(this, Set->VictimHit);
	}
	TemporalFX::Burst(this, GetActorLocation() + FVector(0.f, 0.f, 50.f), FLinearColor(1.f, 0.75f, 0.5f), 0.3f);
}

EEntropiaShotResult ATemporalMannequin::ReceiveBulletHit(const FVector& Direction, float Damage, float Impulse, ATemporalProjectile* Bullet)
{
	if (bKnockedDown || bPendingHit || IsCollapsing())
	{
		return EEntropiaShotResult::Ignored;
	}
	const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
	if (Dilation && Dilation->IsFrozen(this))
	{
		// Congelado: el tiro espera (estilo JoJo, todos salen juntos cuando su tiempo vuelve).
		FPendingShot Shot;
		Shot.Bullet = Bullet;
		Shot.Direction = Direction;
		Shot.Damage = Damage;
		Shot.Impulse = Impulse;
		StoredShots.Add(Shot);
		return EEntropiaShotResult::Stored;
	}
	return ApplyShot(Direction, Damage, Impulse) ? EEntropiaShotResult::KnockedDown : EEntropiaShotResult::Hurt;
}

bool ATemporalMannequin::ApplyShot(const FVector& Direction, float Damage, float Impulse)
{
	Toughness -= Damage;
	if (Toughness <= 0.f)
	{
		Toughness = 0.f;
		KnockDown(Direction, Impulse);
		return true;
	}
	ReceiveHit(Direction, false);
	return false;
}

void ATemporalMannequin::ForgetStoredShot(const ATemporalProjectile* Bullet)
{
	StoredShots.RemoveAll([Bullet](const FPendingShot& Shot) { return Bullet && Shot.Bullet.Get() == Bullet; });
}

void ATemporalMannequin::ReleaseStoredShots()
{
	// Todos los impactos llegan juntos: aguante, empujón y caída de una vez. Las balas pegadas revientan.
	FEntropiaStoredShots Sum;
	for (const FPendingShot& Shot : StoredShots)
	{
		Sum.Add(Shot.Direction, Shot.Damage, Shot.Impulse);
		if (ATemporalProjectile* Bullet = Shot.Bullet.Get())
		{
			TemporalFX::Burst(this, Bullet->GetActorLocation(), FLinearColor(1.f, 0.78f, 0.25f), 0.35f);
			Bullet->Destroy();
		}
	}
	StoredShots.Reset();
	if (Sum.Num() > 0)
	{
		// Con muchos tiros el empujón total sería enorme: se limita para que caiga, no que salga volando del cuarto.
		ApplyShot(Sum.AverageDirection(), Sum.TotalDamage(), FMath::Min(Sum.TotalImpulse(), 2000.f));
	}
}

void ATemporalMannequin::DiscardStoredShots()
{
	for (const FPendingShot& Shot : StoredShots)
	{
		if (ATemporalProjectile* Bullet = Shot.Bullet.Get())
		{
			Bullet->Destroy();
		}
	}
	StoredShots.Reset();
}

void ATemporalMannequin::SetSubdued(bool bInSubdued, float PoseDelay)
{
	bSubdued = bInSubdued;
	SubduedPoseDelay = bInSubdued ? FMath::Max(PoseDelay, 0.f) : 0.f;
	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (!Set)
	{
		return;
	}
	if (bSubdued)
	{
		if (SubduedPoseDelay <= 0.f)
		{
			EntropiaAnim::Play(this, Set->VictimSubdued);
		}
	}
	else if (EntropiaAnim::IsPlaying(this, Set->VictimSubdued))
	{
		EntropiaAnim::Stop(this, 0.2f);
	}
}

void ATemporalMannequin::ReceiveNeckBreak(const FVector& Direction)
{
	if (bKnockedDown || bPendingHit || NeckBreakTime > 0.f)
	{
		return;
	}
	bSubdued = false;
	SubduedPoseDelay = 0.f;
	StaggerTime = 0.f;
	NeckBreakDirection = Direction;
	const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
	if (Dilation && Dilation->IsFrozen(this))
	{
		// Congelado: el crujido queda guardado y cae cuando su tiempo vuelve.
		bPendingNeckBreak = true;
		bPendingHit = true;
		PendingDirection = Direction;
		PendingImpulse = 60.f;
		TemporalFX::Burst(this, GetActorLocation() + FVector(0.f, 0.f, 70.f), FLinearColor(0.6f, 0.92f, 1.f), 0.4f);
		return;
	}
	NeckBreakTime = 0.7f;
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(this, Set->VictimNeckBreak);
	}
}

void ATemporalMannequin::KnockDown(const FVector& Direction, float Impulse)
{
	bKnockedDown = true;
	bPendingHit = false;
	// Tirado ya no está en la llave (quien lo tenía agarrado lo suelta).
	bSubdued = false;
	SubduedPoseDelay = 0.f;
	DownTime = 0.f;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	// El cuerpo que cae no empuja la cámara (si no, se mete en tu cara al rematar de cerca).
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// Así la burbuja de Brecha también lo detecta tirado (la cápsula ya no choca).
	Body->SetGenerateOverlapEvents(true);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();
	Body->AddImpulseToAllBodiesBelow(Direction * Impulse, NAME_None, /*bVelChange=*/true, /*bIncludeSelf=*/true);

	TemporalFX::Burst(this, GetActorLocation(), FLinearColor(1.f, 0.4f, 0.25f), 0.7f);
}

void ATemporalMannequin::Recover(bool bWithEffect)
{
	if (bRagdollFrozen)
	{
		SetRagdollFrozen(false);
	}
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSimulatePhysics(false);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCollisionProfileName(TEXT("CharacterMesh"));
	Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Body->SetRelativeLocationAndRotation(MeshRelativeLocation, MeshRelativeRotation);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetActorLocationAndRotation(HomeLocation, HomeRotation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bKnockedDown = false;
	LookTime = 0.f;
	Toughness = MaxToughness;

	if (bWithEffect)
	{
		// "El tiempo lo regresa": onda hacia adentro y destello cian donde reaparece.
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(HomeLocation, -0.03f, 0.6f, 0.3f);
		}
		TemporalFX::Burst(this, HomeLocation, FLinearColor(0.4f, 0.8f, 1.f), 0.6f);
	}
}

void ATemporalMannequin::SetRagdollFrozen(bool bFrozen)
{
	bRagdollFrozen = bFrozen;
	USkeletalMeshComponent* Body = GetMesh();
	if (bFrozen)
	{
		FrozenLinear.Reset();
		FrozenAngular.Reset();
		for (const FBodyInstance* Bone : Body->Bodies)
		{
			FrozenLinear.Add(Bone ? Bone->GetUnrealWorldVelocity() : FVector::ZeroVector);
			FrozenAngular.Add(Bone ? Bone->GetUnrealWorldAngularVelocityInRadians() : FVector::ZeroVector);
		}
		Body->SetEnableGravity(false);
		Body->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		Body->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		return;
	}
	Body->SetEnableGravity(true);
	Body->WakeAllRigidBodies();
	for (int32 i = 0; i < Body->Bodies.Num() && i < FrozenLinear.Num(); ++i)
	{
		if (FBodyInstance* Bone = Body->Bodies[i])
		{
			Bone->SetLinearVelocity(FrozenLinear[i], false);
			Bone->SetAngularVelocityInRadians(FrozenAngular[i], false);
		}
	}
	FrozenLinear.Reset();
	FrozenAngular.Reset();
}

void ATemporalMannequin::ResetMannequin()
{
	// Los tiros guardados se descartan (no salen después ni dos veces).
	DiscardStoredShots();
	Toughness = MaxToughness;
	bPendingHit = false;
	bPendingNeckBreak = false;
	StaggerTime = 0.f;
	NeckBreakTime = 0.f;
	bSubdued = false;
	SubduedPoseDelay = 0.f;
	if (bKnockedDown)
	{
		Recover(false);
	}
	SetActorLocationAndRotation(HomeLocation, HomeRotation, false, nullptr, ETeleportType::TeleportPhysics);
	LookTime = 0.f;
}

void ATemporalMannequin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// DeltaTime ya trae su tiempo propio: congelado vale 0 (no gira, no se levanta).
	StaggerTime = FMath::Max(0.f, StaggerTime - DeltaTime);
	if (StoredShots.Num() > 0)
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		if (!Dilation || !Dilation->IsFrozen(this))
		{
			ReleaseStoredShots();
		}
	}
	if (bSubdued && SubduedPoseDelay > 0.f)
	{
		// Terminó de verse el desarme: queda en la llave.
		SubduedPoseDelay -= DeltaTime;
		if (SubduedPoseDelay <= 0.f)
		{
			SubduedPoseDelay = 0.f;
			if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
			{
				EntropiaAnim::Play(this, Set->VictimSubdued);
			}
		}
	}
	if (NeckBreakTime > 0.f)
	{
		// Se desploma: al terminar la animación de muerte, muñeco de trapo sin empujón.
		NeckBreakTime -= DeltaTime;
		if (NeckBreakTime <= 0.f)
		{
			NeckBreakTime = 0.f;
			KnockDown(NeckBreakDirection, 60.f);
		}
		return;
	}
	if (bPendingHit)
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		if (!Dilation || !Dilation->IsFrozen(this))
		{
			if (bPendingNeckBreak)
			{
				bPendingNeckBreak = false;
				bPendingHit = false;
				ReceiveNeckBreak(PendingDirection);
				return;
			}
			KnockDown(PendingDirection, PendingImpulse);
		}
		return;
	}
	if (bKnockedDown)
	{
		// Tirado y congelado (Detener o burbuja): el muñeco de trapo se queda quieto en el aire o a media caída.
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const bool bFrozenNow = Dilation && Dilation->IsFrozen(this);
		if (bFrozenNow != bRagdollFrozen)
		{
			SetRagdollFrozen(bFrozenNow);
		}
		if (bRagdollFrozen)
		{
			GetMesh()->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
			GetMesh()->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		}
		DownTime += DeltaTime;
		if (DownTime >= RecoverSeconds)
		{
			Recover(true);
		}
		return;
	}
	if (bLookAround && DeltaTime > 0.f && LookAroundDegrees > 0.f)
	{
		// Vaivén suave: la velocidad máxima de giro es LookAroundSpeed.
		LookTime += DeltaTime;
		const float Omega = FMath::DegreesToRadians(LookAroundSpeed) / FMath::DegreesToRadians(LookAroundDegrees);
		const float Offset = LookAroundDegrees * FMath::Sin(LookTime * Omega);
		SetActorRotation(HomeRotation + FRotator(0.f, Offset, 0.f));
	}
}
