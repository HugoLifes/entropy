// ENTROPÍA · Bala temporal

#include "TemporalProjectile.h"
#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
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

void ATemporalProjectile::StepForward(float DeltaTime)
{
	FVector Start = GetActorLocation();
	const float StepLength = Speed * DeltaTime;
	const FVector End = Start + Dir * StepLength;

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
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
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
