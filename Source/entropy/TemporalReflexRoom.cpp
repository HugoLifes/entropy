// ENTROPÍA · Sala 5 · Reflejos

#include "TemporalReflexRoom.h"
#include "InverseStepComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalHealthComponent.h"
#include "TemporalRecorderComponent.h"
#include "TemporalTurret.h"
#include "TemporalMannequin.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace EntropiaReflex
{
	// Medidas (cm, coordenadas locales). La entrada está en X = 0 y las torretas al fondo, mirando hacia -X.
	constexpr float RoomLength = 2400.f;
	constexpr float RoomHalfWidth = 800.f;
	constexpr float RoomWallHeight = 450.f;
	constexpr float RoomThick = 50.f;

	const FLinearColor FloorColor(0.16f, 0.17f, 0.2f);
	const FLinearColor WallColor(0.24f, 0.25f, 0.29f);
	const FLinearColor PillarColor(0.33f, 0.34f, 0.4f);
	const FLinearColor StripeColor(0.9f, 0.2f, 0.15f);

	const FVector StartLocal(200.f, 0.f, 120.f);
	/** La franja roja: de aquí en adelante te disparan. */
	constexpr float FireZoneStartX = 450.f;
}

ATemporalReflexRoom::ATemporalReflexRoom()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalReflexRoom::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATemporalReflexRoom::AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		return;
	}
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetStaticMesh(CubeMesh);
	Block->SetupAttachment(GetRootComponent());
	Block->SetRelativeLocation(Center);
	Block->SetRelativeScale3D(Size / 100.f);
	// Sin traslape (la burbuja no congela la sala). BlockAll = WorldStatic: tapa la vista de las torretas.
	Block->SetGenerateOverlapEvents(false);
	Block->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
	Block->RegisterComponent();
	AddInstanceComponent(Block);
	TemporalFX::ApplyToAllSlots(Block, TemporalFX::MakeSolid(this, Color));
}

void ATemporalReflexRoom::Build()
{
	using namespace EntropiaReflex;
	const float WallZ = RoomWallHeight * 0.5f;

	// Piso, paredes laterales, pared de la entrada y pared del fondo.
	AddBlock(FVector(RoomLength * 0.5f, 0.f, -RoomThick * 0.5f), FVector(RoomLength, RoomHalfWidth * 2.f, RoomThick), FloorColor);
	for (const float Side : { -1.f, 1.f })
	{
		AddBlock(FVector(RoomLength * 0.5f, Side * (RoomHalfWidth + RoomThick * 0.5f), WallZ), FVector(RoomLength, RoomThick, RoomWallHeight), WallColor);
	}
	AddBlock(FVector(-RoomThick * 0.5f, 0.f, WallZ), FVector(RoomThick, RoomHalfWidth * 2.f + RoomThick * 2.f, RoomWallHeight), WallColor);
	AddBlock(FVector(RoomLength + RoomThick * 0.5f, 0.f, WallZ), FVector(RoomThick, RoomHalfWidth * 2.f + RoomThick * 2.f, RoomWallHeight), WallColor);

	// Franja roja en el piso: de aquí en adelante te disparan.
	AddBlock(FVector(FireZoneStartX, 0.f, 1.f), FVector(10.f, RoomHalfWidth * 2.f, 2.f), StripeColor, false);

	// Columnas para cubrirte.
	const FVector PillarSize(120.f, 120.f, 300.f);
	for (const FVector2D& Spot : { FVector2D(850.f, -420.f), FVector2D(850.f, 420.f), FVector2D(1450.f, -180.f), FVector2D(1450.f, 560.f) })
	{
		AddBlock(FVector(Spot.X, Spot.Y, PillarSize.Z * 0.5f), PillarSize, PillarColor);
	}

	// Tres torretas al fondo mirando a la entrada; empiezan a disparar desfasadas (ritmo, no ráfaga).
	const float FirstShots[] = { 1.f, 1.6f, 2.2f };
	const float Lanes[] = { -500.f, 0.f, 500.f };
	for (int32 Lane = 0; Lane < 3; ++Lane)
	{
		const float Y = Lanes[Lane];
		const FRotator Facing(0.f, GetActorRotation().Yaw + 180.f, 0.f);
		const FTransform T(Facing, GetActorTransform().TransformPosition(FVector(2200.f, Y, 0.f)));
		ATemporalTurret* Turret = GetWorld()->SpawnActorDeferred<ATemporalTurret>(ATemporalTurret::StaticClass(), T, this);
		if (!Turret)
		{
			continue;
		}
		// Más lentas que las de la sala 4 para que se puedan esquivar.
		Turret->BulletSpeed = 1800.f;
		Turret->Range = 3000.f;
		Turret->bTrackPlayer = true;
		Turret->bBulletsHurtPlayer = true;
		Turret->FirstShotDelay = FirstShots[Lane];
		UGameplayStatics::FinishSpawningActor(Turret, T);
		Turrets.Add(Turret);
		TurretStartRotations.Add(Facing);
	}

	// Maniquíes temporales (clones oscuros del jugador): uno fácil en la zona segura, uno vigía y uno de frente.
	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	struct FMannequinSpot
	{
		FVector Local;
		float Yaw;
		bool bLookAround;
	};
	const FMannequinSpot Spots[] = {
		{ FVector(330.f, -520.f, 100.f), -90.f, false },
		{ FVector(1150.f, 300.f, 100.f), 0.f, true },
		{ FVector(1000.f, -150.f, 100.f), 180.f, false },
	};
	for (const FMannequinSpot& Spot : Spots)
	{
		const FTransform T(FRotator(0.f, GetActorRotation().Yaw + Spot.Yaw, 0.f), RoomToWorld(Spot.Local));
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATemporalMannequin* Mannequin = GetWorld()->SpawnActor<ATemporalMannequin>(ATemporalMannequin::StaticClass(), T, Params);
		if (!Mannequin)
		{
			continue;
		}
		Mannequin->bLookAround = Spot.bLookAround;
		if (Player)
		{
			Mannequin->SetupFromPlayer(Player->GetMesh());
		}
		Mannequins.Add(Mannequin);
	}
}

void ATemporalReflexRoom::ResetRoom()
{
	for (int32 i = 0; i < Turrets.Num(); ++i)
	{
		if (ATemporalTurret* Turret = Turrets[i])
		{
			Turret->ClearBullets();
			Turret->SetTrackingEnabled(false);
			Turret->SetActorRotation(TurretStartRotations[i]);
		}
	}
	for (ATemporalMannequin* Mannequin : Mannequins)
	{
		if (Mannequin)
		{
			Mannequin->ResetMannequin();
		}
	}
	bPlayerInside = false;
}

void ATemporalReflexRoom::PlacePlayerAtStart()
{
	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return;
	}
	if (UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>())
	{
		Inverse->EndInverseStep();
	}
	const FVector Target = RoomToWorld(EntropiaReflex::StartLocal);
	Character->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator(-5.f, GetActorRotation().Yaw, 0.f));
	}
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}
	if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
	{
		Health->ResetHealth();
	}
	TemporalFX::Burst(this, Target, FLinearColor(1.f, 0.3f, 0.2f), 0.8f);
}

void ATemporalReflexRoom::TeleportPlayerHere()
{
	ResetRoom();
	PlacePlayerAtStart();
	ShowHint();
}

void ATemporalReflexRoom::RespawnPlayer()
{
	ResetRoom();
	PlacePlayerAtStart();
	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UTemporalHealthComponent* Health = Pawn->FindComponentByClass<UTemporalHealthComponent>())
		{
			Health->SetInvulnerable(1.5f);
		}
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(Pawn->GetActorLocation(), -0.03f, 0.7f, 0.4f);
		}
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1005, 3.f, FColor(255, 90, 70), TEXT("Te dieron. De vuelta a la entrada."));
	}
}

bool ATemporalReflexRoom::IsPlayerInFireZone() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return false;
	}
	using namespace EntropiaReflex;
	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	return Local.X > FireZoneStartX && Local.X < RoomLength && FMath::Abs(Local.Y) < RoomHalfWidth && Local.Z > -200.f && Local.Z < RoomWallHeight + 300.f;
}

void ATemporalReflexRoom::ShowHint() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1004, 30.f, FColor(255, 110, 90),
			TEXT("SALA 5 · REFLEJOS: pasa la franja roja y las torretas te disparan · Shift = dash (justo antes de que te pegue = TIEMPO DE REFLEJO) · Clic der. = apuntar (Ojo lento) · Clic izq. = detener · Por la ESPALDA de un maniqui (o si esta congelado): CLIC = REMATE · 5 = empezar de nuevo"));
	}
}

void ATemporalReflexRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Las torretas solo te siguen mientras estás pasando la franja roja.
	const bool bInside = IsPlayerInFireZone();
	if (bInside != bPlayerInside)
	{
		bPlayerInside = bInside;
		for (ATemporalTurret* Turret : Turrets)
		{
			if (Turret)
			{
				Turret->SetTrackingEnabled(bInside);
			}
		}
	}
}
