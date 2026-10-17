// ENTROPÍA · Sala 7 · Combate

#include "TemporalCombatRoom.h"
#include "EntropiaRoomBuilder.h"
#include "InverseStepComponent.h"
#include "TemporalArmedDummy.h"
#include "TemporalFX.h"
#include "TemporalHealthComponent.h"
#include "TemporalRecorderComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace EntropiaCombatRoom
{
	constexpr float RoomLength = 2600.f;
	constexpr float RoomHalfWidth = 800.f;
	constexpr float RoomWallHeight = 450.f;
	const FLinearColor FloorColor(0.15f, 0.15f, 0.17f);
	const FLinearColor WallColor(0.24f, 0.23f, 0.26f);
	const FLinearColor CoverColor(0.45f, 0.43f, 0.4f);
	const FLinearColor EnemyCoverColor(0.5f, 0.2f, 0.18f);
	const FVector StartLocal(200.f, 0.f, 98.f);
}

ATemporalCombatRoom::ATemporalCombatRoom()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalCombatRoom::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATemporalCombatRoom::Build()
{
	EntropiaRooms::AddShell(this, EntropiaCombatRoom::RoomLength, EntropiaCombatRoom::RoomHalfWidth, EntropiaCombatRoom::RoomWallHeight,
		EntropiaCombatRoom::FloorColor, EntropiaCombatRoom::WallColor);

	// Tu lado: dos bardas bajas y una columna alta.
	EntropiaRooms::AddBlock(this, FVector(700.f, -380.f, 55.f), FVector(60.f, 300.f, 110.f), EntropiaCombatRoom::CoverColor);
	EntropiaRooms::AddBlock(this, FVector(700.f, 380.f, 55.f), FVector(60.f, 300.f, 110.f), EntropiaCombatRoom::CoverColor);
	EntropiaRooms::AddBlock(this, FVector(1150.f, 0.f, 150.f), FVector(100.f, 100.f, 300.f), EntropiaCombatRoom::CoverColor);

	// Su lado: tres muros altos con un dummy armado detrás de cada uno. Se asoman de lado.
	struct FDummySpot
	{
		FVector Wall;
		FVector Home;
		FVector Peek;
	};
	const FDummySpot Spots[] = {
		{ FVector(1750.f, -420.f, 100.f), FVector(1850.f, -420.f, 98.f), FVector(0.f, 170.f, 0.f) },
		{ FVector(1750.f, 420.f, 100.f), FVector(1850.f, 420.f, 98.f), FVector(0.f, -170.f, 0.f) },
		{ FVector(2250.f, 0.f, 100.f), FVector(2350.f, 0.f, 98.f), FVector(0.f, 170.f, 0.f) },
	};
	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	float Delay = 0.f;
	for (const FDummySpot& Spot : Spots)
	{
		EntropiaRooms::AddBlock(this, Spot.Wall, FVector(60.f, 200.f, 200.f), EntropiaCombatRoom::EnemyCoverColor);
		const FTransform T(FRotator(0.f, GetActorRotation().Yaw + 180.f, 0.f), RoomToWorld(Spot.Home));
		ATemporalArmedDummy* Dummy = GetWorld()->SpawnActorDeferred<ATemporalArmedDummy>(ATemporalArmedDummy::StaticClass(), T, this,
			nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Dummy)
		{
			continue;
		}
		Dummy->PeekOffset = GetActorRotation().RotateVector(Spot.Peek);
		Dummy->RoutineDelay = Delay;
		UGameplayStatics::FinishSpawningActor(Dummy, T);
		if (Player)
		{
			Dummy->SetupFromPlayer(Player->GetMesh());
		}
		Dummies.Add(Dummy);
		Delay += 0.8f;
	}
}

void ATemporalCombatRoom::ResetRoom()
{
	for (ATemporalArmedDummy* Dummy : Dummies)
	{
		if (Dummy)
		{
			Dummy->ResetMannequin();
		}
	}
}

void ATemporalCombatRoom::PlacePlayer(const FVector& Local, float YawOffset) const
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

void ATemporalCombatRoom::TeleportPlayerHere()
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
	PlacePlayer(EntropiaCombatRoom::StartLocal);
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}
	if (UTemporalHealthComponent* Health = Character->FindComponentByClass<UTemporalHealthComponent>())
	{
		Health->ResetHealth();
	}
	ShowHint();
	TemporalFX::Burst(this, RoomToWorld(EntropiaCombatRoom::StartLocal), FLinearColor(0.9f, 0.3f, 0.25f), 0.8f);
}

void ATemporalCombatRoom::ShowHint() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1007, 30.f, FColor(255, 120, 100),
			TEXT("SALA 7 · COMBATE: los dummies se asoman y disparan · C junto a una barda = cubrirse (apunta para asomarte) · clic izq. = golpes · E = agarrar y desarmar · por la espalda: CLIC = romper el cuello"));
	}
}

void ATemporalCombatRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const bool bInside = Local.X > 0.f && Local.X < EntropiaCombatRoom::RoomLength && FMath::Abs(Local.Y) < EntropiaCombatRoom::RoomHalfWidth
		&& Local.Z > -200.f && Local.Z < EntropiaCombatRoom::RoomWallHeight + 300.f;
	if (bInside && !bPlayerInside)
	{
		ShowHint();
	}
	bPlayerInside = bInside;
}
