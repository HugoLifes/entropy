// ENTROPÍA · Salas del banco de pruebas

#include "EntropiaRoomsComponent.h"
#include "EntropiaPlayerPowers.h"
#include "TemporalParkourRoom.h"
#include "TemporalCombatRoom.h"
#include "TemporalShootingRoom.h"
#include "TemporalFXSubsystem.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"
#include "TemporalEnergyComponent.h"
#include "TemporalHUDWidget.h"
#include "ObjectInversionComponent.h"
#include "OverdriveComponent.h"
#include "TemporalAimComponent.h"
#include "TimeStopComponent.h"
#include "TemporalDashComponent.h"
#include "TemporalHealthComponent.h"
#include "TemporalReflexRoom.h"
#include "TemporalDilation.h"
#include "SlowEyeComponent.h"
#include "TakedownComponent.h"
#include "TemporalMannequin.h"
#include "InverseStepComponent.h"
#include "EchoGhost.h"
#include "TimeBubble.h"
#include "PressurePlateDoor.h"
#include "TemporalTestChamber.h"
#include "CrumblePlatform.h"
#include "TemporalFX.h"
#include "TemporalBreakable.h"
#include "TemporalPhysicsLab.h"
#include "TemporalTurret.h"
#include "TemporalProjectile.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GenericPlatform/GenericPlatformMisc.h"

UEntropiaRoomsComponent::UEntropiaRoomsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FTransform UEntropiaRoomsComponent::PlaceAt(const FVector& Offset) const
{
	const AActor* Owner = GetOwner();
	return FTransform(FRotator(0.f, Owner->GetActorRotation().Yaw, 0.f), Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(Offset));
}

void UEntropiaRoomsComponent::BuildAll(UEntropiaPlayerPowers* InPowers)
{
	Powers = InPowers;
	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		PlayerStartLocation = Pawn->GetActorLocation();
	}

	if (bSpawnPlateDemo)
	{
		const FTransform PlateTransform(GetOwner()->GetActorRotation(), GetOwner()->GetActorLocation() + GetOwner()->GetActorRotation().RotateVector(PlateOffset));
		GetWorld()->SpawnActor<APressurePlateDoor>(APressurePlateDoor::StaticClass(), PlateTransform);
	}

	SpawnCubes();

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	if (bSpawnTestChamber)
	{
		Chamber = GetWorld()->SpawnActor<ATemporalTestChamber>(ATemporalTestChamber::StaticClass(), PlaceAt(ChamberOffset), Params);
		if (Chamber && bStartInChamber)
		{
			GoToRoom(1);
		}
	}
	if (bSpawnPhysicsLab)
	{
		PhysicsLab = GetWorld()->SpawnActor<ATemporalPhysicsLab>(ATemporalPhysicsLab::StaticClass(), PlaceAt(PhysicsLabOffset), Params);
	}
	if (bSpawnReflexRoom)
	{
		ReflexRoom = GetWorld()->SpawnActor<ATemporalReflexRoom>(ATemporalReflexRoom::StaticClass(), PlaceAt(ReflexRoomOffset), Params);
	}
	if (bSpawnParkourRoom)
	{
		ParkourRoom = GetWorld()->SpawnActor<ATemporalParkourRoom>(ATemporalParkourRoom::StaticClass(), PlaceAt(ParkourRoomOffset), Params);
	}
	if (bSpawnCombatRoom)
	{
		CombatRoom = GetWorld()->SpawnActor<ATemporalCombatRoom>(ATemporalCombatRoom::StaticClass(), PlaceAt(CombatRoomOffset), Params);
	}
	if (bSpawnShootingRoom)
	{
		ShootingRoom = GetWorld()->SpawnActor<ATemporalShootingRoom>(ATemporalShootingRoom::StaticClass(), PlaceAt(ShootingRoomOffset), Params);
	}
}

TArray<AActor*> UEntropiaRoomsComponent::GatherRewindables() const
{
	// Todo lo que tenga Registro Temporal, menos el jugador (él usa F): cajas, plataformas,
	// pedazos de objetos rotos, balas… Así cada cosa nueva con Recorder responde a R sin tocar este código.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	TArray<AActor*> Result;
	if (const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>())
	{
		for (UTemporalRecorderComponent* Recorder : Subsystem->GetRecorders())
		{
			AActor* RecorderOwner = Recorder->GetOwner();
			if (RecorderOwner != Player && IsValid(RecorderOwner))
			{
				Result.AddUnique(RecorderOwner);
			}
		}
	}
	return Result;
}

void UEntropiaRoomsComponent::RewindAll(bool bStart)
{
	for (AActor* Actor : GatherRewindables())
	{
		if (UTemporalRecorderComponent* Recorder = Actor->FindComponentByClass<UTemporalRecorderComponent>())
		{
			if (bStart)
			{
				Recorder->StartRewind();
			}
			else
			{
				Recorder->StopRewind();
			}
		}
	}
}

void UEntropiaRoomsComponent::GoToRoom(int32 Key)
{
	LastRoom = Key;
	switch (Key)
	{
	case 0:
		BackToStart();
		return;
	case 4:
		if (PhysicsLab)
		{
			Powers->EndForTeleport();
			Powers->ClearEchoes();
			PhysicsLab->TeleportPlayerHere();
		}
		return;
	case 5:
		if (ReflexRoom)
		{
			Powers->EndForTeleport();
			Powers->ClearEchoes();
			ReflexRoom->TeleportPlayerHere();
		}
		return;
	case 6:
		if (ParkourRoom)
		{
			Powers->EndForTeleport();
			Powers->ClearEchoes();
			ParkourRoom->TeleportPlayerHere();
		}
		return;
	case 7:
		if (CombatRoom)
		{
			Powers->EndForTeleport();
			Powers->ClearEchoes();
			CombatRoom->TeleportPlayerHere();
		}
		return;
	case 8:
		if (ShootingRoom)
		{
			Powers->EndForTeleport();
			Powers->ClearEchoes();
			ShootingRoom->TeleportPlayerHere();
		}
		return;
	default:
		break;
	}
	// 1, 2, 3: salas del cuarto de pruebas.
	if (Chamber)
	{
		Powers->EndForTeleport();
		Powers->ClearEchoes();
		Chamber->TeleportToRoom(Key - 1);
	}
}

void UEntropiaRoomsComponent::BackToStart()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Powers->EndForTeleport();
		Powers->ClearEchoes();
		Pawn->SetActorLocation(PlayerStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
		// El Paso Inverso deja la velocidad del pasado; al teletransportarte no queremos que te deslices.
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		if (Powers->Recorder)
		{
			Powers->Recorder->ClearRecording();
		}
	}
}

void UEntropiaRoomsComponent::RespawnPlayer()
{
	if (LastRoom == 5 && ReflexRoom)
	{
		ReflexRoom->RespawnPlayer();
		return;
	}
	// En cualquier otra sala: de vuelta a su entrada, con la vida llena y un momento de protección.
	GoToRoom(LastRoom);
	if (Powers && Powers->Health)
	{
		Powers->Health->ResetHealth();
		Powers->Health->SetInvulnerable(1.5f);
	}
}

void UEntropiaRoomsComponent::Fire()
{
	if (PhysicsLab && PhysicsLab->GetTurret())
	{
		PhysicsLab->GetTurret()->Fire();
	}
}

void UEntropiaRoomsComponent::FireInverted()
{
	if (PhysicsLab && PhysicsLab->GetTurret())
	{
		PhysicsLab->GetTurret()->FireInverted();
	}
}

void UEntropiaRoomsComponent::BreakInFront()
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}

	// El rompible entero más cercano que esté enfrente (a menos de 6 m).
	const FVector Eye = Pawn->GetActorLocation();
	const FVector Forward = Pawn->GetActorForwardVector();
	ATemporalBreakable* Best = nullptr;
	float BestDist = 600.f;
	for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
	{
		const FVector ToIt = It->GetActorLocation() - Eye;
		const float Dist = ToIt.Size();
		if (!It->IsBroken() && Dist < BestDist && FVector::DotProduct(ToIt.GetSafeNormal(), Forward) > 0.4f)
		{
			Best = *It;
			BestDist = Dist;
		}
	}
	if (Best)
	{
		Best->Break(Best->GetActorLocation() - Forward * 50.f, Forward);
	}
}

void UEntropiaRoomsComponent::SpawnCubes()
{
	for (AActor* Cube : Cubes)
	{
		if (IsValid(Cube))
		{
			Cube->Destroy();
		}
	}
	Cubes.Reset();

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		return;
	}

	for (int32 i = 0; i < NumCubes; ++i)
	{
		const FVector Location = GetOwner()->GetActorLocation() + FVector(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f), SpawnHeight + i * 110.f);
		const FRotator Rotation(FMath::FRandRange(0.f, 90.f), FMath::FRandRange(0.f, 90.f), 0.f);

		AStaticMeshActor* Cube = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Cube)
		{
			continue;
		}

		UStaticMeshComponent* Mesh = Cube->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetGenerateOverlapEvents(true);
		Mesh->SetSimulatePhysics(true);

		UTemporalRecorderComponent* Recorder = NewObject<UTemporalRecorderComponent>(Cube, TEXT("TemporalRecorder"));
		Recorder->RegisterComponent();
		Cube->AddInstanceComponent(Recorder);

		Cubes.Add(Cube);
	}
}
