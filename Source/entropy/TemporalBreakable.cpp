// ENTROPÍA · Objeto que se rompe y se rearma

#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATemporalBreakable::ATemporalBreakable()
{
	// Tick solo para contar la invulnerabilidad tras rearmarse.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// Necesario para enterarnos cuando algo con física nos golpea.
	Mesh->SetNotifyRigidBodyCollision(true);
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	SetRootComponent(Mesh);
}

void ATemporalBreakable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetWorldScale3D(Size / 100.f);
}

void ATemporalBreakable::BeginPlay()
{
	Super::BeginPlay();
	Mesh->SetWorldScale3D(Size / 100.f);
	Mesh->OnComponentHit.AddDynamic(this, &ATemporalBreakable::OnHit);
	Material = TemporalFX::MakeSolid(this, Color);
	TemporalFX::ApplyToAllSlots(Mesh, Material);
}

void ATemporalBreakable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyShards();
	Super::EndPlay(EndPlayReason);
}

void ATemporalBreakable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Invulnerable = FMath::Max(0.f, Invulnerable - DeltaTime);
}

void ATemporalBreakable::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bBroken || Invulnerable > 0.f || !OtherComp || !OtherComp->IsSimulatingPhysics() || (OtherActor && OtherActor->IsA<APawn>()))
	{
		return;
	}

	// El evento llega después del choque: la velocidad del otro ya bajó. El impulso / masa ≈ cuánto lo frenamos.
	const float Mass = FMath::Max(1.f, OtherComp->GetMass());
	const float ImpactSpeed = FMath::Max(OtherComp->GetPhysicsLinearVelocity().Size(), NormalImpulse.Size() / Mass);
	if (ImpactSpeed >= BreakSpeed)
	{
		Break(Hit.ImpactPoint, OtherComp->GetPhysicsLinearVelocity().GetSafeNormal());
	}
}

void ATemporalBreakable::Break(FVector ImpactPoint, FVector Direction)
{
	if (bBroken)
	{
		return;
	}

	UStaticMesh* CubeMesh = Mesh->GetStaticMesh();
	if (!CubeMesh)
	{
		return;
	}

	bBroken = true;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DestroyShards();

	const FIntVector Count(FMath::Max(1, Pieces.X), FMath::Max(1, Pieces.Y), FMath::Max(1, Pieces.Z));
	const FVector PieceSize(Size.X / Count.X, Size.Y / Count.Y, Size.Z / Count.Z);
	// Sin escala: el actor está escalado a Size/100 y Local ya viene en centímetros.
	const FTransform Whole(GetActorRotation(), GetActorLocation());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 X = 0; X < Count.X; ++X)
	{
		for (int32 Y = 0; Y < Count.Y; ++Y)
		{
			for (int32 Z = 0; Z < Count.Z; ++Z)
			{
				const FVector Local = -Size * 0.5f + PieceSize * FVector(X + 0.5f, Y + 0.5f, Z + 0.5f);
				const FVector Location = Whole.TransformPosition(Local);

				AStaticMeshActor* Shard = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Whole.Rotator(), Params);
				if (!Shard)
				{
					continue;
				}

				UStaticMeshComponent* ShardMesh = Shard->GetStaticMeshComponent();
				ShardMesh->SetMobility(EComponentMobility::Movable);
				ShardMesh->SetStaticMesh(CubeMesh);
				// Un poco más chicos que su hueco para que no se estorben al nacer ni al rearmarse.
				ShardMesh->SetWorldScale3D(PieceSize / 100.f * 0.96f);
				ShardMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
				ShardMesh->SetSimulatePhysics(true);
				TemporalFX::ApplyToAllSlots(ShardMesh, Material);

				UTemporalRecorderComponent* Recorder = NewObject<UTemporalRecorderComponent>(Shard, TEXT("TemporalRecorder"));
				Recorder->MaxRecordSeconds = ShardMemorySeconds;
				Recorder->RegisterComponent();
				Shard->AddInstanceComponent(Recorder);
				Recorder->OnRewindFinished.AddDynamic(this, &ATemporalBreakable::HandleShardRewindFinished);

				// Salen disparados desde el punto del golpe, empujados también hacia donde iba el golpe.
				const FVector Away = (Location - ImpactPoint).GetSafeNormal();
				const FVector Push = (Away + Direction.GetSafeNormal() * 0.6f + FMath::VRand() * 0.25f).GetSafeNormal() * BlastStrength;
				ShardMesh->AddImpulse(Push, NAME_None, /*bVelChange=*/true);
				ShardMesh->AddAngularImpulseInDegrees(FMath::VRand() * 360.f, NAME_None, /*bVelChange=*/true);

				Shards.Add(Shard);
				ShardHomes.Add(Location);
			}
		}
	}

	TemporalFX::Burst(this, ImpactPoint, FLinearColor(1.f, 0.6f, 0.2f), 0.8f);
}

void ATemporalBreakable::RewindShards(float Speed)
{
	for (AStaticMeshActor* Shard : Shards)
	{
		if (UTemporalRecorderComponent* Recorder = IsValid(Shard) ? Shard->FindComponentByClass<UTemporalRecorderComponent>() : nullptr)
		{
			Recorder->RewindSpeed = Speed;
			Recorder->StartRewind();
		}
	}
}

void ATemporalBreakable::HandleShardRewindFinished()
{
	if (!bBroken)
	{
		return;
	}

	// La bala invertida rebobina rápido; quien ya terminó vuelve a velocidad normal para la R del jugador.
	for (AStaticMeshActor* Shard : Shards)
	{
		if (UTemporalRecorderComponent* Recorder = IsValid(Shard) ? Shard->FindComponentByClass<UTemporalRecorderComponent>() : nullptr)
		{
			if (!Recorder->IsRewinding())
			{
				Recorder->RewindSpeed = 1.f;
			}
		}
	}

	// Solo cuando TODOS terminaron de rebobinar y quedaron cerca de su hueco.
	for (int32 i = 0; i < Shards.Num(); ++i)
	{
		const AStaticMeshActor* Shard = Shards[i];
		const UTemporalRecorderComponent* Recorder = IsValid(Shard) ? Shard->FindComponentByClass<UTemporalRecorderComponent>() : nullptr;
		if (!Recorder || Recorder->IsRewinding() || FVector::Dist(Shard->GetActorLocation(), ShardHomes[i]) > 25.f)
		{
			return;
		}
	}
	Reassemble();
}

void ATemporalBreakable::Reassemble()
{
	DestroyShards();
	bBroken = false;
	Invulnerable = 0.4f;
	Mesh->SetVisibility(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.3f, 0.7f, 1.f), 0.8f);
}

void ATemporalBreakable::Restore()
{
	if (bBroken)
	{
		Reassemble();
	}
}

void ATemporalBreakable::DestroyShards()
{
	for (AStaticMeshActor* Shard : Shards)
	{
		if (IsValid(Shard))
		{
			Shard->Destroy();
		}
	}
	Shards.Reset();
	ShardHomes.Reset();
}

bool ATemporalBreakable::SegmentCrosses(const FVector& A, const FVector& B) const
{
	// Pasamos el segmento al espacio local del bloque (así sirve aunque esté rotado).
	const FTransform Unscaled(GetActorRotation(), GetActorLocation());
	const FVector LocalA = Unscaled.InverseTransformPosition(A);
	const FVector LocalB = Unscaled.InverseTransformPosition(B);
	const FBox Box(-Size * 0.5f, Size * 0.5f);
	const FVector Delta = LocalB - LocalA;
	if (Delta.IsNearlyZero())
	{
		return Box.IsInside(LocalA);
	}
	return FMath::LineBoxIntersection(Box, LocalA, LocalB, Delta);
}
