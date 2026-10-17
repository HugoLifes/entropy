// ENTROPÍA · Bala temporal

#include "TemporalProjectile.h"
#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalCombat.h"
#include "TemporalHealthComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/WorldSettings.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor NormalColor(1.f, 0.55f, 0.1f);
	const FLinearColor InvertedColor(0.2f, 0.6f, 1.f);
}

ATemporalProjectile::ATemporalProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	// Solo traslape: no estorba a nadie, pero la burbuja de Brecha sí la detecta.
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCastShadow(false);
	// Esfera estirada = trazador de 35 cm.
	Mesh->SetRelativeScale3D(FVector(0.35f, 0.1f, 0.1f));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}
	SetRootComponent(Mesh);

	Recorder = CreateDefaultSubobject<UTemporalRecorderComponent>(TEXT("Recorder"));
	Recorder->MaxRecordSeconds = 10.f;
}

void ATemporalProjectile::BeginPlay()
{
	Super::BeginPlay();
	Recorder->OnRewindFinished.AddDynamic(this, &ATemporalProjectile::HandleRewindFinished);
}

void ATemporalProjectile::Paint(const FLinearColor& Color)
{
	TemporalFX::ApplyToAllSlots(Mesh, TemporalFX::MakeSolid(this, Color));
}

void ATemporalProjectile::ResetIgnored()
{
	Pierced.Reset();
}

void ATemporalProjectile::Launch(const FVector& InMuzzle, const FVector& Direction, AActor* InGun)
{
	Muzzle = InMuzzle;
	Dir = Direction.GetSafeNormal();
	Gun = InGun;
	Traveled = 0.f;
	ResetIgnored();
	SetActorLocationAndRotation(Muzzle, Dir.Rotation());
	Paint(NormalColor);
	State = EState::Flying;
}

void ATemporalProjectile::LaunchInverted(const FVector& InMuzzle, const FVector& Impact, AActor* InGun)
{
	Muzzle = InMuzzle;
	Dir = (Impact - InMuzzle).GetSafeNormal();
	Gun = InGun;
	Wait = InvertedDelay;
	SetActorLocationAndRotation(Impact, Dir.Rotation());
	Paint(InvertedColor);
	TemporalFX::Burst(this, Impact, InvertedColor, 0.6f);
	State = EState::InvertedWaiting;
}

void ATemporalProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Mientras el jugador la rebobina, manda el Registro Temporal.
	if (Recorder->IsRewinding())
	{
		return;
	}

	switch (State)
	{
	case EState::Flying:
		StepForward(DeltaTime);
		break;
	case EState::InvertedWaiting:
		Wait -= DeltaTime;
		if (Wait <= 0.f)
		{
			State = EState::InvertedFlying;
		}
		break;
	case EState::InvertedFlying:
		StepInverted(DeltaTime);
		break;
	default:
		break;
	}
}

FVector ATemporalProjectile::GetRealVelocity() const
{
	if (Recorder->IsRewinding())
	{
		return FVector::ZeroVector;
	}
	const AWorldSettings* Settings = GetWorldSettings();
	const float Global = Settings ? Settings->GetEffectiveTimeDilation() : 1.f;
	const float Scale = Speed * CustomTimeDilation * Global;
	switch (State)
	{
	case EState::Flying:
		return Dir * Scale;
	case EState::InvertedFlying:
		return -Dir * Scale;
	default:
		return FVector::ZeroVector;
	}
}

bool ATemporalProjectile::HitPlayer(ACharacter* Victim, const FVector& Where)
{
	UTemporalHealthComponent* Health = Victim->FindComponentByClass<UTemporalHealthComponent>();
	if (!Health || !Health->ApplyDamage(Damage))
	{
		return false;
	}
	Victim->LaunchCharacter(Dir * PlayerPush + FVector(0.f, 0.f, 120.f), true, false);
	TemporalFX::Burst(this, Where, FLinearColor(1.f, 0.15f, 0.1f), 0.5f);
	Destroy();
	return true;
}

void ATemporalProjectile::StepForward(float DeltaTime)
{
	FVector Start = GetActorLocation();
	const float StepLength = Speed * DeltaTime;
	const FVector End = Start + Dir * StepLength;

	// ¿Este tramo toca al jugador? Las paredes solo se revisan hasta ahí (una pared en medio te protege).
	ACharacter* Victim = bHurtsPlayer ? Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)) : nullptr;
	float PlayerDistance = -1.f;
	if (Victim && !Pierced.Contains(Victim))
	{
		const UCapsuleComponent* Capsule = Victim->GetCapsuleComponent();
		float Fraction = 0.f;
		if (TemporalCombat::SegmentHitsCapsule(Start, End, Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), Fraction))
		{
			PlayerDistance = Fraction * StepLength;
		}
	}
	const FVector TraceEnd = PlayerDistance >= 0.f ? Start + Dir * PlayerDistance : End;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TemporalProjectile), false, this);
	auto Ignore = [&Params](const AActor* Actor)
	{
		if (Actor)
		{
			Params.AddIgnoredActor(Actor);
		}
	};
	Ignore(Gun.Get());
	Ignore(UGameplayStatics::GetPlayerPawn(this, 0));
	for (const TWeakObjectPtr<AActor>& Actor : Pierced)
	{
		Ignore(Actor.Get());
	}

	// Pocas vueltas: cada cosa atravesada se ignora y seguimos el mismo tramo.
	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, TraceEnd, ECC_Visibility, Params))
		{
			break;
		}

		AActor* HitActor = Hit.GetActor();
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (ATemporalBreakable* Breakable = Cast<ATemporalBreakable>(HitActor))
		{
			Breakable->Break(Hit.ImpactPoint, Dir);
			// La bala y lo que rompió quedan en el mismo grupo: al invertir la bala, el muro se rearma con ella.
			for (UTemporalRecorderComponent* Shard : Breakable->GetShardRecorders())
			{
				Recorder->LinkWith(Shard);
			}
		}
		else if (HitComp && HitComp->IsSimulatingPhysics())
		{
			HitComp->AddImpulseAtLocation(Dir * PushSpeed * HitComp->GetMass(), Hit.ImpactPoint);
			TemporalFX::Burst(this, Hit.ImpactPoint, NormalColor, 0.3f);
		}
		else if (HitActor && HitActor->IsA<APawn>())
		{
			// Enemigos y ecos todavía no reciben daño: la bala los atraviesa.
		}
		else
		{
			// Pared o piso: se incrusta.
			Embed(Hit.ImpactPoint);
			return;
		}

		Pierced.Add(HitActor);
		Ignore(HitActor);
		Start = Hit.ImpactPoint;
	}

	if (PlayerDistance >= 0.f)
	{
		if (HitPlayer(Victim, TraceEnd))
		{
			return;
		}
		// Invulnerable (dash): la bala te atraviesa.
		Pierced.Add(Victim);
	}

	SetActorLocation(End);
	Traveled += StepLength;
	if (Traveled > Range)
	{
		Destroy();
	}
}

void ATemporalProjectile::StepInverted(float DeltaTime)
{
	const FVector Previous = GetActorLocation();
	FVector Next = Previous - Dir * Speed * DeltaTime;
	// ¿Ya pasamos el cañón?
	const bool bArrived = FVector::DotProduct(Next - Muzzle, Dir) <= 0.f;
	if (bArrived)
	{
		Next = Muzzle;
	}

	// Al pasar por donde estaba un bloque roto, "des-rompe" su pasado (rebobinado rápido).
	for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
	{
		if (It->IsBroken() && It->SegmentCrosses(Previous, Next))
		{
			It->RewindShards(4.f);
		}
	}

	SetActorLocation(Next);
	if (bArrived)
	{
		ReturnToGun();
	}
}

void ATemporalProjectile::Embed(const FVector& At)
{
	SetActorLocation(At - Dir * 5.f);
	State = EState::Embedded;
	TemporalFX::Burst(this, At, NormalColor, 0.4f);
}

void ATemporalProjectile::ReturnToGun()
{
	TemporalFX::Burst(this, Muzzle, IsInverted() ? InvertedColor : NormalColor, 0.5f);
	Destroy();
}

void ATemporalProjectile::HandleRewindFinished()
{
	if (IsInverted())
	{
		// Rebobinar una bala invertida la lleva de vuelta a la pared; al soltar sigue su camino al cañón.
		return;
	}

	// Rebobinada hasta el cañón: la bala "nunca se disparó".
	if (FVector::Dist(GetActorLocation(), Muzzle) < 80.f)
	{
		ReturnToGun();
		return;
	}

	// Soltaste R a medio camino: el tiempo sigue y la bala vuelve a volar hacia adelante.
	Traveled = FVector::Dist(GetActorLocation(), Muzzle);
	ResetIgnored();
	State = EState::Flying;
}
