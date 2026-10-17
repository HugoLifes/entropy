// ENTROPÍA · Dummy armado

#include "TemporalArmedDummy.h"
#include "EntropiaAnimationSet.h"
#include "TemporalDilation.h"
#include "TemporalFX.h"
#include "TemporalProjectile.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Rutina (lógica pura)

int32 FArmedDummyRoutine::Advance(float Dt)
{
	if (Dt <= 0.f)
	{
		return 0;
	}
	Time += Dt;
	int32 NewShots = 0;
	for (int32 Guard = 0; Guard < 8; ++Guard)
	{
		switch (Step)
		{
		case EStep::Covered:
			if (Time < CoverSeconds)
			{
				return NewShots;
			}
			Time -= CoverSeconds;
			Step = EStep::Stepping;
			break;
		case EStep::Stepping:
			if (Time < StepSeconds)
			{
				return NewShots;
			}
			Time -= StepSeconds;
			Step = EStep::Firing;
			ShotsFired = 0;
			break;
		case EStep::Firing:
		{
			// Un disparo al salir y luego uno cada ShotInterval.
			const int32 Due = FMath::Min(Shots, FMath::FloorToInt(Time / ShotInterval) + 1);
			NewShots += FMath::Max(0, Due - ShotsFired);
			ShotsFired = FMath::Max(ShotsFired, Due);
			const float Done = (Shots - 1) * ShotInterval + LingerSeconds;
			if (ShotsFired < Shots || Time < Done)
			{
				return NewShots;
			}
			Time -= Done;
			Step = EStep::Returning;
			break;
		}
		case EStep::Returning:
			if (Time < ReturnSeconds)
			{
				return NewShots;
			}
			Time -= ReturnSeconds;
			Step = EStep::Covered;
			break;
		}
	}
	return NewShots;
}

float FArmedDummyRoutine::StepAlpha() const
{
	switch (Step)
	{
	case EStep::Stepping: return StepSeconds > 0.f ? FMath::Clamp(Time / StepSeconds, 0.f, 1.f) : 1.f;
	case EStep::Firing: return 1.f;
	case EStep::Returning: return ReturnSeconds > 0.f ? 1.f - FMath::Clamp(Time / ReturnSeconds, 0.f, 1.f) : 0.f;
	default: return 0.f;
	}
}

void FArmedDummyRoutine::Reset()
{
	Step = EStep::Covered;
	Time = 0.f;
	ShotsFired = 0;
}

// ---------------------------------------------------------------------------
// Actor

ATemporalArmedDummy::ATemporalArmedDummy()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(GetCapsuleComponent());
	Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gun->SetGenerateOverlapEvents(false);
	Gun->SetRelativeScale3D(FVector(0.26f, 0.05f, 0.11f));
	if (Cube.Succeeded())
	{
		Gun->SetStaticMesh(Cube.Object);
	}
}

void ATemporalArmedDummy::BeginPlay()
{
	Super::BeginPlay();
	CoverHome = GetActorLocation();
	RestartRoutine();
	TemporalFX::ApplyToAllSlots(Gun, TemporalFX::MakeSolid(this, FLinearColor(0.05f, 0.05f, 0.06f)));
	PlaceGun(false);
}

void ATemporalArmedDummy::PlaceGun(bool bAiming)
{
	if (!bArmed)
	{
		return;
	}
	// Apuntando: frente al pecho, a la altura de los ojos. Cubierto: abajo, junto a la cadera.
	Gun->SetRelativeLocationAndRotation(bAiming ? FVector(55.f, 12.f, 48.f) : FVector(22.f, 22.f, -8.f),
		bAiming ? FRotator::ZeroRotator : FRotator(-60.f, 0.f, 0.f));
}

void ATemporalArmedDummy::FireAtPlayer()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || !bArmed)
	{
		return;
	}
	const FVector Muzzle = Gun->GetComponentLocation() + Gun->GetForwardVector() * 18.f;
	const FVector Target = Player->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	// Solo si te ve (las paredes y columnas te protegen).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArmedDummySight), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, Target, ECC_Visibility, Params) && Hit.GetActor() != Player)
	{
		return;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATemporalProjectile* Bullet = GetWorld()->SpawnActor<ATemporalProjectile>(ATemporalProjectile::StaticClass(), Muzzle, FRotator::ZeroRotator, SpawnParams))
	{
		Bullet->Speed = BulletSpeed;
		Bullet->Damage = BulletDamage;
		Bullet->bHurtsPlayer = true;
		Bullet->Range = 3500.f;
		Bullet->Launch(Muzzle, (Target - Muzzle).GetSafeNormal(), this);
		TemporalFX::Burst(this, Muzzle, FLinearColor(1.f, 0.55f, 0.1f), 0.3f);
		++TotalShots;
		if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
		{
			// El disparo reemplaza la pose de apuntar un momento; al terminar, el Tick la vuelve a poner.
			const float FireSeconds = EntropiaAnim::Play(this, Set->DummyFire);
			if (FireSeconds > 0.f)
			{
				FireAnimLeft = FireSeconds;
				bAimPosePlaying = false;
			}
		}
	}
}

float ATemporalArmedDummy::Disarm(const FVector& Direction)
{
	if (!bArmed)
	{
		return 0.f;
	}
	bArmed = false;
	// La reacción reemplaza la pose de apuntar (ya no hay que quitarla después).
	bAimPosePlaying = false;
	FireAnimLeft = 0.f;
	float ReactionSeconds = 0.f;
	Gun->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Gun->SetCollisionProfileName(TEXT("PhysicsActor"));
	Gun->SetSimulatePhysics(true);
	Gun->AddImpulse((Direction + FVector(0.f, 0.f, 0.8f)) * 350.f, NAME_None, /*bVelChange=*/true);
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		ReactionSeconds = EntropiaAnim::Play(this, Set->VictimDisarmed);
	}
	RestartRoutine();
	SetActorLocation(CoverHome);
	return ReactionSeconds;
}

void ATemporalArmedDummy::ReceiveReturnedBullet(const FVector& Direction)
{
	if (!bArmed || IsKnockedDown() || IsCollapsing())
	{
		return;
	}
	TemporalFX::Burst(this, Gun->GetComponentLocation(), FLinearColor(1.f, 0.55f, 0.1f), 0.8f);
	ReceiveBulletHit(Direction, MaxToughness * 2.f, 900.f, nullptr);
}

void ATemporalArmedDummy::RestartRoutine()
{
	Routine.Reset();
	Routine.Time = -RoutineDelay;
}

void ATemporalArmedDummy::ReceiveHit(const FVector& Direction, bool bKnockdown)
{
	Super::ReceiveHit(Direction, bKnockdown);
	// Un golpe le corta la rutina: vuelve a empezar cubierto.
	RestartRoutine();
	bAimPosePlaying = false;
	FireAnimLeft = 0.f;
}

void ATemporalArmedDummy::ResetMannequin()
{
	Super::ResetMannequin();
	if (!bArmed)
	{
		Gun->SetSimulatePhysics(false);
		Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Gun->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		bArmed = true;
	}
	RestartRoutine();
	bAimPosePlaying = false;
	FireAnimLeft = 0.f;
	SetActorLocation(CoverHome);
	PlaceGun(false);
}

void ATemporalArmedDummy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	// Congelado (Detener, burbuja, mundo detenido): nada cambia, ni su pose (aunque tenga un golpe guardado). No se asoma,
	// no gira hacia ti y no dispara.
	if (const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this); Dilation && Dilation->IsFrozen(this))
	{
		return;
	}
	// Quita la pose de apuntar o de disparo solo si es la que tiene puesta: si una reacción (desarme, cuello roto, golpe)
	// ya la reemplazó, no hay que cortarla.
	auto StopOwnPose = [this, Set]()
	{
		if (Set && (EntropiaAnim::IsPlaying(this, Set->DummyAim) || EntropiaAnim::IsPlaying(this, Set->DummyFire)))
		{
			EntropiaAnim::Stop(this, 0.2f);
		}
		bAimPosePlaying = false;
		FireAnimLeft = 0.f;
	};
	if (!bArmed || IsReacting())
	{
		RestartRoutine();
		StopOwnPose();
		return;
	}

	// DeltaTime ya trae su tiempo propio.
	FireAnimLeft = FMath::Max(0.f, FireAnimLeft - DeltaTime);
	const int32 Shots = Routine.Advance(DeltaTime);
	const float Out = Routine.StepAlpha();
	if (DeltaTime > 0.f)
	{
		SetActorLocation(CoverHome + PeekOffset * Out);
	}

	const bool bOut = Out > 0.01f;
	if (bOut)
	{
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && DeltaTime > 0.f)
		{
			const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
			SetActorRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));
		}
		if (!bAimPosePlaying && FireAnimLeft <= 0.f && Set)
		{
			EntropiaAnim::Play(this, Set->DummyAim);
			bAimPosePlaying = true;
		}
	}
	else if (bAimPosePlaying || FireAnimLeft > 0.f)
	{
		StopOwnPose();
	}
	PlaceGun(bOut);

	for (int32 i = 0; i < Shots; ++i)
	{
		FireAtPlayer();
	}
}
