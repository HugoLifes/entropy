// ENTROPÍA · Sala 8 · Tiroteo

#include "TemporalShootingRoom.h"
#include "EntropiaRoomBuilder.h"
#include "InverseStepComponent.h"
#include "TemporalArmedDummy.h"
#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalHealthComponent.h"
#include "TemporalRecorderComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace EntropiaShootingRoom
{
	constexpr float RoomLength = 3600.f;
	constexpr float RoomHalfWidth = 900.f;
	constexpr float RoomWallHeight = 600.f;
	const FLinearColor FloorColor(0.13f, 0.13f, 0.15f);
	const FLinearColor WallColor(0.22f, 0.22f, 0.25f);
	const FLinearColor CoverColor(0.45f, 0.43f, 0.4f);
	const FLinearColor EnemyCoverColor(0.5f, 0.2f, 0.18f);
	const FLinearColor PanelColor(0.55f, 0.5f, 0.42f);
	const FLinearColor CrateColor(0.6f, 0.42f, 0.22f);
	const FVector StartLocal(200.f, 0.f, 98.f);
}

ATemporalShootingRoom::ATemporalShootingRoom()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalShootingRoom::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

ATemporalBreakable* ATemporalShootingRoom::AddBreakable(const FVector& Center, const FVector& Size, const FIntVector& Pieces, const FLinearColor& Color)
{
	const FTransform T(GetActorRotation(), RoomToWorld(Center));
	ATemporalBreakable* Breakable = GetWorld()->SpawnActorDeferred<ATemporalBreakable>(ATemporalBreakable::StaticClass(), T, this);
	if (Breakable)
	{
		Breakable->Size = Size;
		Breakable->Pieces = Pieces;
		Breakable->Color = Color;
		UGameplayStatics::FinishSpawningActor(Breakable, T);
	}
	return Breakable;
}

void ATemporalShootingRoom::Build()
{
	EntropiaRooms::AddShell(this, EntropiaShootingRoom::RoomLength, EntropiaShootingRoom::RoomHalfWidth, EntropiaShootingRoom::RoomWallHeight, EntropiaShootingRoom::FloorColor, EntropiaShootingRoom::WallColor);

	// Tus coberturas: dos bardas bajas cerca de la entrada.
	EntropiaRooms::AddBlock(this, FVector(500.f, -400.f, 55.f), FVector(60.f, 300.f, 110.f), EntropiaShootingRoom::CoverColor);
	EntropiaRooms::AddBlock(this, FVector(500.f, 400.f, 55.f), FVector(60.f, 300.f, 110.f), EntropiaShootingRoom::CoverColor);

	// Sus muros: 4 dummies a 12, 18, 25 y 30 m, alternando de lado. Se asoman hacia el centro.
	struct FDummySpot
	{
		float X;
		float Y;
	};
	const FDummySpot Spots[] = { { 1200.f, -450.f }, { 1800.f, 450.f }, { 2500.f, -300.f }, { 3000.f, 300.f } };
	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	float Delay = 0.f;
	for (const FDummySpot& Spot : Spots)
	{
		EntropiaRooms::AddBlock(this, FVector(Spot.X - 100.f, Spot.Y, 100.f), FVector(60.f, 200.f, 200.f), EntropiaShootingRoom::EnemyCoverColor);
		const FTransform T(FRotator(0.f, GetActorRotation().Yaw + 180.f, 0.f), RoomToWorld(FVector(Spot.X, Spot.Y, 98.f)));
		ATemporalArmedDummy* Dummy = GetWorld()->SpawnActorDeferred<ATemporalArmedDummy>(ATemporalArmedDummy::StaticClass(), T, this,
			nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Dummy)
		{
			continue;
		}
		Dummy->PeekOffset = GetActorRotation().RotateVector(FVector(0.f, Spot.Y > 0.f ? -170.f : 170.f, 0.f));
		Dummy->RoutineDelay = Delay;
		UGameplayStatics::FinishSpawningActor(Dummy, T);
		if (Player)
		{
			Dummy->SetupFromPlayer(Player->GetMesh());
		}
		Dummies.Add(Dummy);
		Delay += 0.6f;
	}

	// Un muro rompible frente al dummy de 18 m (las balas lo atraviesan y, al invertirlas, se rearma).
	if (ATemporalBreakable* Panel = AddBreakable(FVector(1650.f, 450.f, 100.f), FVector(40.f, 300.f, 200.f), FIntVector(1, 3, 2), EntropiaShootingRoom::PanelColor))
	{
		Breakables.Add(Panel);
	}
	// Cajas en medio: se rompen a balazos.
	for (const FVector& Crate : { FVector(900.f, 0.f, 50.f), FVector(1400.f, -200.f, 50.f), FVector(2100.f, 250.f, 50.f) })
	{
		if (ATemporalBreakable* Box = AddBreakable(Crate, FVector(100.f, 100.f, 100.f), FIntVector(2, 2, 2), EntropiaShootingRoom::CrateColor))
		{
			Crates.Add(Box);
		}
	}
}

void ATemporalShootingRoom::ResetRoom()
{
	for (ATemporalArmedDummy* Dummy : Dummies)
	{
		if (Dummy)
		{
			Dummy->ResetMannequin();
		}
	}
	for (TArray<TObjectPtr<ATemporalBreakable>>* List : { &Breakables, &Crates })
	{
		for (ATemporalBreakable* Breakable : *List)
		{
			if (IsValid(Breakable))
			{
				Breakable->Restore();
			}
		}
	}
}

void ATemporalShootingRoom::PlacePlayer(const FVector& Local, float YawOffset) const
{
	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return;
	}
	const float Yaw = GetActorRotation().Yaw + YawOffset;
	Character->SetActorLocationAndRotation(RoomToWorld(Local), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator(-8.f, Yaw, 0.f));
	}
}

void ATemporalShootingRoom::TeleportPlayerHere()
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
	ResetRoom();
	PlacePlayer(EntropiaShootingRoom::StartLocal);
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}
	if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
	{
		Health->ResetHealth();
	}
	ShowHint();
	TemporalFX::Burst(this, RoomToWorld(EntropiaShootingRoom::StartLocal), FLinearColor(1.f, 0.78f, 0.25f), 0.8f);
}

void ATemporalShootingRoom::ShowHint() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1009, 30.f, FColor(255, 200, 90),
			TEXT("SALA 8 · TIROTEO: apunta + clic = disparar · rueda: arma · N + Q: LLUVIA DETENIDA · R a bala enemiga: DEVUÉLVESELA · R a tus balas: RECARGA INVERSA"));
	}
}

void ATemporalShootingRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const bool bInside = Local.X > 0.f && Local.X < EntropiaShootingRoom::RoomLength && FMath::Abs(Local.Y) < EntropiaShootingRoom::RoomHalfWidth
		&& Local.Z > -200.f && Local.Z < EntropiaShootingRoom::RoomWallHeight + 300.f;
	if (bInside && !bPlayerInside)
	{
		ShowHint();
	}
	bPlayerInside = bInside;
}
