// ENTROPÍA · Sala 6 · Parkour

#include "TemporalParkourRoom.h"
#include "EntropiaRoomBuilder.h"
#include "InverseStepComponent.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"

#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace EntropiaParkourRoom
{
	constexpr float RoomLength = 3000.f;
	constexpr float RoomHalfWidth = 800.f;
	constexpr float RoomWallHeight = 450.f;
	const FLinearColor FloorColor(0.17f, 0.19f, 0.18f);
	const FLinearColor WallColor(0.26f, 0.28f, 0.27f);
	const FLinearColor ObstacleColor(0.75f, 0.6f, 0.3f);
	const FLinearColor HighColor(0.4f, 0.55f, 0.75f);
	const FVector StartLocal(150.f, 0.f, 120.f);
}

ATemporalParkourRoom::ATemporalParkourRoom()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalParkourRoom::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATemporalParkourRoom::Build()
{
	EntropiaRooms::AddShell(this, EntropiaParkourRoom::RoomLength, EntropiaParkourRoom::RoomHalfWidth, EntropiaParkourRoom::RoomWallHeight,
		EntropiaParkourRoom::FloorColor, EntropiaParkourRoom::WallColor);

	// Carril izquierdo (Y = -450): barda, mesa, plataforma baja.
	EntropiaRooms::AddBlock(this, FVector(600.f, -450.f, 50.f), FVector(40.f, 300.f, 100.f), EntropiaParkourRoom::ObstacleColor);
	EntropiaRooms::AddBlock(this, FVector(1200.f, -450.f, 50.f), FVector(150.f, 300.f, 100.f), EntropiaParkourRoom::ObstacleColor);
	EntropiaRooms::AddBlock(this, FVector(2200.f, -450.f, 55.f), FVector(800.f, 300.f, 110.f), EntropiaParkourRoom::ObstacleColor);

	// Carril central (Y = 150): muro de 2 m y muro de 3 m (con plataforma arriba).
	EntropiaRooms::AddBlock(this, FVector(1100.f, 150.f, 100.f), FVector(600.f, 400.f, 200.f), EntropiaParkourRoom::HighColor);
	EntropiaRooms::AddBlock(this, FVector(2200.f, 150.f, 150.f), FVector(600.f, 400.f, 300.f), EntropiaParkourRoom::HighColor);
}

void ATemporalParkourRoom::PlacePlayer(const FVector& Local) const
{
	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return;
	}
	Character->SetActorLocation(RoomToWorld(Local), false, nullptr, ETeleportType::TeleportPhysics);
	Character->SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator(-12.f, GetActorRotation().Yaw, 0.f));
	}
}

void ATemporalParkourRoom::TeleportPlayerHere()
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
	PlacePlayer(EntropiaParkourRoom::StartLocal);
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}
	ShowHint();
	TemporalFX::Burst(this, RoomToWorld(EntropiaParkourRoom::StartLocal), FLinearColor(0.9f, 0.7f, 0.3f), 0.8f);
}

void ATemporalParkourRoom::ShowHint() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1006, 30.f, FColor(240, 190, 90),
			TEXT("SALA 6 · PARKOUR: ESPACIO frente a algo = saltarlo, subir o trepar · corriendo (Shift) sobre la mesa = el gato · corriendo hacia el muro alto = correr por la pared · C = agacharse"));
	}
}

void ATemporalParkourRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const bool bInside = Local.X > 0.f && Local.X < EntropiaParkourRoom::RoomLength && FMath::Abs(Local.Y) < EntropiaParkourRoom::RoomHalfWidth
		&& Local.Z > -200.f && Local.Z < EntropiaParkourRoom::RoomWallHeight + 600.f;
	if (bInside && !bPlayerInside)
	{
		ShowHint();
	}
	bPlayerInside = bInside;
}
