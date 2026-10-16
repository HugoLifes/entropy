// ENTROPÍA · Caja de arena temporal

#include "TemporalSandbox.h"
#include "TemporalRecorderComponent.h"
#include "InverseStepComponent.h"
#include "EchoGhost.h"
#include "TimeBubble.h"
#include "PressurePlateDoor.h"
#include "TemporalTestChamber.h"
#include "CrumblePlatform.h"
#include "TemporalFX.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATemporalSandbox::ATemporalSandbox()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalSandbox::BeginPlay()
{
	Super::BeginPlay();
	// Esperamos un frame para que el jugador ya exista.
	GetWorldTimerManager().SetTimerForNextTick(this, &ATemporalSandbox::Setup);
}

void ATemporalSandbox::Setup()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ATemporalSandbox::OnInversePressed);
			InputComponent->BindKey(EKeys::F, IE_Released, this, &ATemporalSandbox::OnInverseReleased);
			InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ATemporalSandbox::OnRewindPressed);
			InputComponent->BindKey(EKeys::R, IE_Released, this, &ATemporalSandbox::OnRewindReleased);
			InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATemporalSandbox::OnEcho);
			InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ATemporalSandbox::OnClearEchoes);
			InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ATemporalSandbox::OnBubble);
			InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ATemporalSandbox::OnRespawnCubes);
			InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ATemporalSandbox::OnRoom1);
			InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ATemporalSandbox::OnRoom2);
			InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ATemporalSandbox::OnRoom3);
			InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ATemporalSandbox::OnBackToStart);
		}
	}

	// El jugador graba su posición y su pose (la usan el Eco y el Paso Inverso).
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		PlayerStartLocation = Pawn->GetActorLocation();
		PlayerRecorder = Pawn->FindComponentByClass<UTemporalRecorderComponent>();
		if (!PlayerRecorder)
		{
			PlayerRecorder = NewObject<UTemporalRecorderComponent>(Pawn, TEXT("PlayerTemporalRecorder"));
			PlayerRecorder->MaxRecordSeconds = EchoSeconds;
			PlayerRecorder->bRecordPose = true;
			PlayerRecorder->RegisterComponent();
		}

		PlayerInverseStep = Pawn->FindComponentByClass<UInverseStepComponent>();
		if (!PlayerInverseStep)
		{
			PlayerInverseStep = NewObject<UInverseStepComponent>(Pawn, TEXT("PlayerInverseStep"));
			PlayerInverseStep->RegisterComponent();
		}
	}

	if (bSpawnPlateDemo)
	{
		const FTransform PlateTransform(GetActorRotation(), GetActorLocation() + GetActorRotation().RotateVector(PlateOffset));
		GetWorld()->SpawnActor<APressurePlateDoor>(APressurePlateDoor::StaticClass(), PlateTransform);
	}

	SpawnCubes();
	ShowHelp();

	if (bSpawnTestChamber)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		const FTransform ChamberTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), GetActorLocation() + GetActorRotation().RotateVector(ChamberOffset));
		Chamber = GetWorld()->SpawnActor<ATemporalTestChamber>(ATemporalTestChamber::StaticClass(), ChamberTransform, Params);
		if (Chamber && bStartInChamber)
		{
			GoToRoom(0);
		}
	}
}

TArray<AActor*> ATemporalSandbox::GatherRewindables() const
{
	TArray<AActor*> Result;
	for (AActor* Cube : Cubes)
	{
		if (IsValid(Cube))
		{
			Result.Add(Cube);
		}
	}
	if (Chamber)
	{
		for (ACrumblePlatform* Platform : Chamber->GetPlatforms())
		{
			if (IsValid(Platform))
			{
				Result.Add(Platform);
			}
		}
	}
	return Result;
}

void ATemporalSandbox::OnRoom1() { GoToRoom(0); }
void ATemporalSandbox::OnRoom2() { GoToRoom(1); }
void ATemporalSandbox::OnRoom3() { GoToRoom(2); }

void ATemporalSandbox::GoToRoom(int32 RoomIndex)
{
	if (Chamber)
	{
		OnClearEchoes();
		Chamber->TeleportToRoom(RoomIndex);
	}
}

void ATemporalSandbox::OnBackToStart()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (PlayerInverseStep)
		{
			PlayerInverseStep->EndInverseStep();
		}
		OnClearEchoes();
		Pawn->SetActorLocation(PlayerStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
		// El Paso Inverso deja la velocidad del pasado; al teletransportarte no queremos que te deslices.
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
		if (PlayerRecorder)
		{
			PlayerRecorder->ClearRecording();
		}
	}
}

void ATemporalSandbox::SpawnCubes()
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
		const FVector Location = GetActorLocation() + FVector(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f), SpawnHeight + i * 110.f);
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

void ATemporalSandbox::ShowHelp()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1000, 40.f, FColor::Cyan,
			TEXT("ENTROPIA · F (mantener): Paso Inverso · E: eco (max 3) · G: borrar ecos · R (mantener): rebobinar objetos · Q: burbuja · T: soltar cajas"));
		GEngine->AddOnScreenDebugMessage(1002, 40.f, FColor::Green,
			TEXT("CUARTO DE PRUEBAS · 1: sala Eco · 2: sala Inverso · 3: sala Brecha · 0: volver a las cajas"));
	}
}

void ATemporalSandbox::OnInversePressed()
{
	if (PlayerInverseStep)
	{
		PlayerInverseStep->BeginInverseStep();
	}
}

void ATemporalSandbox::OnInverseReleased()
{
	if (PlayerInverseStep)
	{
		PlayerInverseStep->EndInverseStep();
	}
}

void ATemporalSandbox::OnRewindPressed()
{
	for (AActor* Actor : GatherRewindables())
	{
		if (UTemporalRecorderComponent* Recorder = Actor->FindComponentByClass<UTemporalRecorderComponent>())
		{
			Recorder->StartRewind();
		}
	}
}

void ATemporalSandbox::OnRewindReleased()
{
	for (AActor* Actor : GatherRewindables())
	{
		if (UTemporalRecorderComponent* Recorder = Actor->FindComponentByClass<UTemporalRecorderComponent>())
		{
			Recorder->StopRewind();
		}
	}
}

void ATemporalSandbox::OnEcho()
{
	if (!PlayerRecorder || PlayerRecorder->IsRewinding() || PlayerRecorder->GetRecordedSeconds() < 0.5f)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const TArray<FTemporalFrame> Frames = PlayerRecorder->GetRecording();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEchoGhost* Ghost = GetWorld()->SpawnActor<AEchoGhost>(AEchoGhost::StaticClass(), Frames[0].Transform, Params);
	if (!Ghost)
	{
		return;
	}

	if (Character)
	{
		const UCapsuleComponent* PlayerCapsule = Character->GetCapsuleComponent();
		Ghost->SetupFromCharacterMesh(Character->GetMesh(), PlayerCapsule->GetUnscaledCapsuleRadius(), PlayerCapsule->GetUnscaledCapsuleHalfHeight());
	}
	Ghost->StartReplay(Frames, true);

	// Coro: máximo N ecos; el más viejo desaparece.
	Echoes.RemoveAll([](const TObjectPtr<AEchoGhost>& E) { return !IsValid(E); });
	Echoes.Add(Ghost);
	while (Echoes.Num() > MaxEchoes)
	{
		if (IsValid(Echoes[0]))
		{
			Echoes[0]->Destroy();
		}
		Echoes.RemoveAt(0);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1001, 2.f, FColor(120, 200, 255),
			FString::Printf(TEXT("Eco %d/%d: repitiendo %.1f s en bucle"), Echoes.Num(), MaxEchoes, PlayerRecorder->GetRecordedSeconds()));
	}
}

void ATemporalSandbox::OnClearEchoes()
{
	for (AEchoGhost* Ghost : Echoes)
	{
		if (IsValid(Ghost))
		{
			Ghost->Destroy();
		}
	}
	Echoes.Reset();
}

void ATemporalSandbox::OnBubble()
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}

	const FVector Location = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * BubbleRadius;
	const FTransform Transform(FRotator::ZeroRotator, Location);

	ATimeBubble* Bubble = GetWorld()->SpawnActorDeferred<ATimeBubble>(ATimeBubble::StaticClass(), Transform);
	if (!Bubble)
	{
		return;
	}
	Bubble->Dilation = BubbleDilation;
	Bubble->LifeSeconds = BubbleSeconds;
	Bubble->Sphere->SetSphereRadius(BubbleRadius);
	UGameplayStatics::FinishSpawningActor(Bubble, Transform);
}

void ATemporalSandbox::OnRespawnCubes()
{
	SpawnCubes();
}
