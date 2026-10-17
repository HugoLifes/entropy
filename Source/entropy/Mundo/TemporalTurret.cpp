// ENTROPÍA · Torreta de pruebas

#include "TemporalTurret.h"
#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalProjectile.h"
#include "TemporalSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ATemporalTurret::ATemporalTurret()
{
	// Solo tickea mientras sigue al jugador (sala 5).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(GetRootComponent());
	Base->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
	Base->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.5f));
	Base->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// Así la burbuja de Brecha la detecta y la congela.
	Base->SetGenerateOverlapEvents(true);
	if (Cube.Succeeded())
	{
		Base->SetStaticMesh(Cube.Object);
	}

	// Cañón: cilindro acostado a lo largo de +X.
	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(GetRootComponent());
	Barrel->SetRelativeLocation(FVector(45.f, 0.f, 60.f));
	Barrel->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Barrel->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.9f));
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Cylinder.Succeeded())
	{
		Barrel->SetStaticMesh(Cylinder.Object);
	}

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(GetRootComponent());
	Muzzle->SetRelativeLocation(FVector(95.f, 0.f, 60.f));
}

void ATemporalTurret::BeginPlay()
{
	Super::BeginPlay();
	TemporalFX::ApplyToAllSlots(Base, TemporalFX::MakeSolid(this, FLinearColor(0.15f, 0.15f, 0.18f)));
	TemporalFX::ApplyToAllSlots(Barrel, TemporalFX::MakeSolid(this, FLinearColor(0.6f, 0.15f, 0.1f)));
	if (bTrackPlayer)
	{
		if (UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>())
		{
			Subsystem->RegisterTimeTarget(this);
		}
	}
}

void ATemporalTurret::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTemporalSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UTemporalSubsystem>() : nullptr)
	{
		Subsystem->UnregisterTimeTarget(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ATemporalTurret::SetTrackingEnabled(bool bEnabled)
{
	if (bTrackingEnabled == bEnabled)
	{
		return;
	}
	bTrackingEnabled = bEnabled && bTrackPlayer;
	// El primer disparo tarda un poco más: te da tiempo de reaccionar.
	FireTimer = FireInterval - FirstShotDelay;
	SetActorTickEnabled(bTrackingEnabled);
}

void ATemporalTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// DeltaTime ya viene con su tiempo propio: congelada vale 0 y no gira ni dispara.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!bTrackingEnabled || !Player || DeltaTime <= 0.f)
	{
		return;
	}

	// Apunta al pecho del jugador.
	const FVector Target = Player->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	const FVector From = Muzzle->GetComponentLocation();
	const FVector ToTarget = Target - From;
	const float DesiredYaw = ToTarget.Rotation().Yaw;
	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, DesiredYaw, TurnRate * DeltaTime);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));

	FireTimer += DeltaTime;
	if (FireTimer < FireInterval)
	{
		return;
	}

	// ¿Ya te apunta y te ve? (las balas solo se traslapan: no tapan la vista)
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, DesiredYaw));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TemporalTurretSight), false, this);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, From, Target, ECC_Visibility, Params) && Hit.GetActor() != Player;
	if (YawError <= AimTolerance && !bBlocked)
	{
		FireTimer = 0.f;
		FireTowards(ToTarget.GetSafeNormal());
	}
}

ATemporalProjectile* ATemporalTurret::SpawnBullet(const FVector& Location)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATemporalProjectile* Bullet = GetWorld()->SpawnActor<ATemporalProjectile>(ATemporalProjectile::StaticClass(), Location, GetActorRotation(), Params);
	if (!Bullet)
	{
		return nullptr;
	}
	Bullet->Speed = BulletSpeed;
	Bullet->Range = Range;

	Bullets.RemoveAll([](const TWeakObjectPtr<ATemporalProjectile>& B) { return !B.IsValid(); });
	Bullets.Add(Bullet);
	while (Bullets.Num() > MaxBullets)
	{
		if (Bullets[0].IsValid())
		{
			Bullets[0]->Destroy();
		}
		Bullets.RemoveAt(0);
	}
	return Bullet;
}

void ATemporalTurret::Fire()
{
	FireTowards(GetActorForwardVector());
}

void ATemporalTurret::FireTowards(const FVector& Direction)
{
	const FVector From = Muzzle->GetComponentLocation();
	if (ATemporalProjectile* Bullet = SpawnBullet(From))
	{
		Bullet->bHurtsPlayer = bBulletsHurtPlayer;
		Bullet->Launch(From, Direction, this);
		TemporalFX::Burst(this, From, FLinearColor(1.f, 0.55f, 0.1f), 0.4f);
	}
}

void ATemporalTurret::FireInverted()
{
	// La bala invertida "ya pegó": buscamos dónde. Ignoramos los bloques rompibles: en su pasado la bala los atravesó.
	const FVector From = Muzzle->GetComponentLocation();
	const FVector To = From + GetActorForwardVector() * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TemporalTurretInverted), false, this);
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->IsA<ATemporalBreakable>() || It->IsA<ATemporalProjectile>())
		{
			Params.AddIgnoredActor(*It);
		}
	}

	// Solo paredes (WorldStatic). Así no cuentan los objetos con física (PhysicsBody) ni las zonas de
	// traslape como la burbuja o los detectores (WorldDynamic).
	FCollisionObjectQueryParams Walls;
	Walls.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, From, To, Walls, Params);
	const FVector Impact = bHit ? FVector(Hit.ImpactPoint) - GetActorForwardVector() * 5.f : To;

	if (ATemporalProjectile* Bullet = SpawnBullet(Impact))
	{
		Bullet->LaunchInverted(From, Impact, this);
	}
}

void ATemporalTurret::ClearBullets()
{
	for (const TWeakObjectPtr<ATemporalProjectile>& Bullet : Bullets)
	{
		if (Bullet.IsValid())
		{
			Bullet->Destroy();
		}
	}
	Bullets.Reset();
}
