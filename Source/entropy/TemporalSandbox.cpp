// ENTROPÍA · Caja de arena temporal

#include "TemporalSandbox.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"
#include "TemporalEnergyComponent.h"
#include "TemporalHUDWidget.h"
#include "ObjectInversionComponent.h"
#include "OverdriveComponent.h"
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
			InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ATemporalSandbox::OnRoom4);
			InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ATemporalSandbox::OnFire);
			InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ATemporalSandbox::OnFireInverted);
			InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ATemporalSandbox::OnBreakInFront);
			InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ATemporalSandbox::OnOverdrive);
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

		// La energía va primero: todos los poderes la buscan al empezar.
		PlayerEnergy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(Pawn, TEXT("PlayerTemporalEnergy"));

		PlayerInverseStep = Pawn->FindComponentByClass<UInverseStepComponent>();
		if (!PlayerInverseStep)
		{
			PlayerInverseStep = NewObject<UInverseStepComponent>(Pawn, TEXT("PlayerInverseStep"));
			PlayerInverseStep->RegisterComponent();
		}

		PlayerInversion = TemporalFX::FindOrAddComponent<UObjectInversionComponent>(Pawn, TEXT("PlayerObjectInversion"));
		PlayerOverdrive = TemporalFX::FindOrAddComponent<UOverdriveComponent>(Pawn, TEXT("PlayerOverdrive"));

		// Barra de energía en pantalla.
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			EnergyHUD = CreateWidget<UTemporalHUDWidget>(PC, UTemporalHUDWidget::StaticClass());
			if (EnergyHUD)
			{
				EnergyHUD->SetEnergy(PlayerEnergy);
				EnergyHUD->AddToViewport();
			}
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

	if (bSpawnPhysicsLab)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		const FTransform LabTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), GetActorLocation() + GetActorRotation().RotateVector(PhysicsLabOffset));
		PhysicsLab = GetWorld()->SpawnActor<ATemporalPhysicsLab>(ATemporalPhysicsLab::StaticClass(), LabTransform, Params);
	}
}

TArray<AActor*> ATemporalSandbox::GatherRewindables() const
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

void ATemporalSandbox::OnRoom4()
{
	if (PhysicsLab)
	{
		EndPlayerPowersForTeleport();
		OnClearEchoes();
		PhysicsLab->TeleportPlayerHere();
	}
}

void ATemporalSandbox::OnFire()
{
	if (PhysicsLab && PhysicsLab->GetTurret())
	{
		PhysicsLab->GetTurret()->Fire();
	}
}

void ATemporalSandbox::OnFireInverted()
{
	if (PhysicsLab && PhysicsLab->GetTurret())
	{
		PhysicsLab->GetTurret()->FireInverted();
	}
}

void ATemporalSandbox::OnBreakInFront()
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

void ATemporalSandbox::OnRoom1() { GoToRoom(0); }
void ATemporalSandbox::OnRoom2() { GoToRoom(1); }
void ATemporalSandbox::OnRoom3() { GoToRoom(2); }

void ATemporalSandbox::GoToRoom(int32 RoomIndex)
{
	if (Chamber)
	{
		EndPlayerPowersForTeleport();
		OnClearEchoes();
		Chamber->TeleportToRoom(RoomIndex);
	}
}

void ATemporalSandbox::OnBackToStart()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		EndPlayerPowersForTeleport();
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
			TEXT("ENTROPIA · F (mantener): Paso Inverso · R (mantener): invertir lo que apuntas · Shift+R: todo · V: Sobremarcha · E: eco · G: borrar ecos · Q: burbuja · T: soltar cajas"));
		GEngine->AddOnScreenDebugMessage(1002, 40.f, FColor::Green,
			TEXT("SALAS · 1: Eco · 2: Inverso · 3: Brecha · 4: Fisica (Z dispara · X bala invertida · B romper) · 0: volver a las cajas"));
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
	// Shift+R = rebobinar TODO (para pruebas, sin costo). R sola = Invertir objeto: solo lo que apuntas.
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	bRewindingAll = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
	if (!bRewindingAll)
	{
		if (PlayerInversion)
		{
			PlayerInversion->BeginInversion();
		}
		return;
	}

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
	if (!bRewindingAll)
	{
		if (PlayerInversion)
		{
			PlayerInversion->EndInversion();
		}
		return;
	}

	bRewindingAll = false;
	for (AActor* Actor : GatherRewindables())
	{
		if (UTemporalRecorderComponent* Recorder = Actor->FindComponentByClass<UTemporalRecorderComponent>())
		{
			Recorder->StopRewind();
		}
	}
}

void ATemporalSandbox::OnOverdrive()
{
	if (PlayerOverdrive)
	{
		PlayerOverdrive->Toggle();
	}
}

void ATemporalSandbox::EndPlayerPowersForTeleport()
{
	if (PlayerInverseStep)
	{
		PlayerInverseStep->EndInverseStep();
	}
	if (PlayerInversion)
	{
		PlayerInversion->EndInversion();
	}
}

void ATemporalSandbox::OnEcho()
{
	if (!PlayerRecorder || PlayerRecorder->IsRewinding() || PlayerRecorder->GetRecordedSeconds() < 0.5f)
	{
		return;
	}
	if (PlayerEnergy && !PlayerEnergy->Spend(EchoCost))
	{
		PlayerEnergy->Deny();
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
	if (PlayerEnergy && !PlayerEnergy->Spend(BubbleCost))
	{
		PlayerEnergy->Deny();
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
