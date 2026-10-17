// ENTROPÍA · Caja de arena temporal

#include "TemporalSandbox.h"
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
#include "CrumblePlatform.h"
#include "TemporalProjectile.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GenericPlatform/GenericPlatformMisc.h"

ATemporalSandbox::ATemporalSandbox()
{
	// Solo tickea durante el modo tour.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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
			InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ATemporalSandbox::OnRoom5);
			InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ATemporalSandbox::OnFire);
			InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ATemporalSandbox::OnFireInverted);
			InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ATemporalSandbox::OnBreakInFront);
			InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ATemporalSandbox::OnOverdrive);
			InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ATemporalSandbox::OnAimPressed);
			InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ATemporalSandbox::OnAimReleased);
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATemporalSandbox::OnPrimaryPressed);
			InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &ATemporalSandbox::OnDash);
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
		PlayerHealth = TemporalFX::FindOrAddComponent<UTemporalHealthComponent>(Pawn, TEXT("PlayerHealth"));
		PlayerHealth->OnDeath.AddUObject(this, &ATemporalSandbox::OnPlayerDied);

		PlayerInverseStep = Pawn->FindComponentByClass<UInverseStepComponent>();
		if (!PlayerInverseStep)
		{
			PlayerInverseStep = NewObject<UInverseStepComponent>(Pawn, TEXT("PlayerInverseStep"));
			PlayerInverseStep->RegisterComponent();
		}

		// La mira va antes que los poderes que la usan (Invertir, Detener).
		PlayerAim = TemporalFX::FindOrAddComponent<UTemporalAimComponent>(Pawn, TEXT("PlayerAim"));
		PlayerInversion = TemporalFX::FindOrAddComponent<UObjectInversionComponent>(Pawn, TEXT("PlayerObjectInversion"));
		PlayerTimeStop = TemporalFX::FindOrAddComponent<UTimeStopComponent>(Pawn, TEXT("PlayerTimeStop"));
		PlayerDash = TemporalFX::FindOrAddComponent<UTemporalDashComponent>(Pawn, TEXT("PlayerDash"));
		PlayerSlowEye = TemporalFX::FindOrAddComponent<USlowEyeComponent>(Pawn, TEXT("PlayerSlowEye"));
		PlayerTakedown = TemporalFX::FindOrAddComponent<UTakedownComponent>(Pawn, TEXT("PlayerTakedown"));
		PlayerOverdrive = TemporalFX::FindOrAddComponent<UOverdriveComponent>(Pawn, TEXT("PlayerOverdrive"));

		// Barra de energía en pantalla.
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			EnergyHUD = CreateWidget<UTemporalHUDWidget>(PC, UTemporalHUDWidget::StaticClass());
			if (EnergyHUD)
			{
				EnergyHUD->SetEnergy(PlayerEnergy);
				EnergyHUD->SetAim(PlayerAim);
				EnergyHUD->SetHealth(PlayerHealth);
				EnergyHUD->SetDash(PlayerDash);
				EnergyHUD->SetTakedown(PlayerTakedown);
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

	if (bSpawnReflexRoom)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		const FTransform RoomTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), GetActorLocation() + GetActorRotation().RotateVector(ReflexRoomOffset));
		ReflexRoom = GetWorld()->SpawnActor<ATemporalReflexRoom>(ATemporalReflexRoom::StaticClass(), RoomTransform, Params);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("EntropiaTour")))
	{
		StartTour();
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

void ATemporalSandbox::OnRoom5()
{
	if (ReflexRoom)
	{
		EndPlayerPowersForTeleport();
		OnClearEchoes();
		ReflexRoom->TeleportPlayerHere();
	}
}

void ATemporalSandbox::OnPlayerDied()
{
	GetWorldTimerManager().SetTimerForNextTick(this, &ATemporalSandbox::RespawnAfterDeath);
}

void ATemporalSandbox::RespawnAfterDeath()
{
	// Morir suelta todo: F, R, Sobremarcha, apuntar, lo detenido, el dash, el tiempo de reflejo y el remate
	// (nada queda pegado ni en cámara lenta al reaparecer).
	EndPlayerPowersForTeleport();
	if (PlayerOverdrive)
	{
		PlayerOverdrive->StopOverdrive();
	}
	if (PlayerTimeStop)
	{
		PlayerTimeStop->ReleaseAll();
	}
	if (PlayerDash)
	{
		PlayerDash->CancelDash();
		PlayerDash->CancelReflex();
	}
	if (PlayerTakedown)
	{
		PlayerTakedown->Cancel();
	}
	if (PlayerAim)
	{
		PlayerAim->SetAiming(false);
	}
	OnClearEchoes();
	if (ReflexRoom)
	{
		ReflexRoom->RespawnPlayer();
	}
	else
	{
		OnBackToStart();
		if (PlayerHealth)
		{
			PlayerHealth->ResetHealth();
		}
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
			TEXT("ENTROPIA · Clic der.: apuntar · Clic izq.: detener · Shift: dash · F (mantener): Paso Inverso · R (mantener): invertir lo que apuntas · Ctrl+R: todo · V: Sobremarcha · E: eco · G: borrar ecos · Q: burbuja · T: soltar cajas"));
		GEngine->AddOnScreenDebugMessage(1002, 40.f, FColor::Green,
			TEXT("SALAS · 1: Eco · 2: Inverso · 3: Brecha · 4: Fisica (Z dispara · X bala invertida · B romper) · 5: Reflejos · 0: volver a las cajas"));
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
	// Ctrl+R = rebobinar TODO (para pruebas, sin costo). R sola = Invertir objeto: solo lo que apuntas.
	// (Antes era Shift+R; Shift ahora es el dash.)
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	bRewindingAll = PC && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl));
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

void ATemporalSandbox::OnAimPressed()
{
	if (PlayerAim)
	{
		PlayerAim->SetAiming(true);
	}
}

void ATemporalSandbox::OnAimReleased()
{
	if (PlayerAim)
	{
		PlayerAim->SetAiming(false);
	}
}

void ATemporalSandbox::OnPrimaryPressed()
{
	// Si el HUD muestra "CLIC: REMATE", el clic remata; si no, detiene lo que está en la mira.
	// Durante un remate los clics extra no hacen nada (antes detenían al maniquí a medio golpe).
	if (PlayerTakedown && (PlayerTakedown->IsBusy() || PlayerTakedown->TryTakedown()))
	{
		return;
	}
	if (PlayerTimeStop)
	{
		PlayerTimeStop->StopAimed();
	}
}

void ATemporalSandbox::OnDash()
{
	if (PlayerDash)
	{
		PlayerDash->TryDash();
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
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Location, 0.03f, 0.6f, 0.35f);
	}
}

void ATemporalSandbox::OnRespawnCubes()
{
	SpawnCubes();
}

// ---------------------------------------------------------------------------
// Modo tour: la IA prueba el juego sola (capturas + FPS por paso)

void ATemporalSandbox::TourRewindAll(bool bStart)
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

void ATemporalSandbox::StartTour()
{
	FString Mode;
	FParse::Value(FCommandLine::Get(), TEXT("EntropiaTour="), Mode);
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR start mode=%s"), Mode.IsEmpty() ? TEXT("normal") : *Mode);
	if (Mode.Equals(TEXT("caos"), ESearchCase::IgnoreCase))
	{
		BuildChaosSteps();
		TourIndex = 0;
		SetActorTickEnabled(true);
		BeginTourStep();
		return;
	}
	auto Add = [this](const TCHAR* Name, float Seconds, TFunction<void()> Action)
	{
		TourSteps.Add({ Name, MoveTemp(Action), Seconds });
	};

	// Bala más reciente en el mundo (la que acaba de disparar la torreta).
	auto NewestBullet = [this]() -> ATemporalProjectile*
	{
		ATemporalProjectile* Best = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			if (!Best || It->GetGameTimeSinceCreation() < Best->GetGameTimeSinceCreation())
			{
				Best = *It;
			}
		}
		return Best;
	};
	auto Player = [this]() { return Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); };
	auto PlaceInLab = [this, Player](const FVector& Local)
	{
		if (PhysicsLab && Player())
		{
			Player()->SetActorLocation(PhysicsLab->GetActorTransform().TransformPosition(Local), false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* Controller = Player()->GetController())
			{
				Controller->SetControlRotation(FRotator(-10.f, PhysicsLab->GetActorRotation().Yaw, 0.f));
			}
		}
	};

	Add(TEXT("inicio"), 2.5f, [] {});
	Add(TEXT("sala4"), 2.f, [this] { OnRoom4(); });
	// Fase 3: apuntar a la caja de la línea de tiro (cámara al hombro, la mira se pone ámbar y la caja se resalta).
	Add(TEXT("apuntar"), 1.6f, [this, Player]
	{
		ACharacter* Character = Player();
		AController* Controller = Character ? Character->GetController() : nullptr;
		if (PhysicsLab && Controller)
		{
			const FVector Crate = PhysicsLab->GetActorTransform().TransformPosition(FVector(800.f, 0.f, 50.f));
			Controller->SetControlRotation((Crate - Character->GetActorLocation() - FVector(0.f, 0.f, 60.f)).Rotation());
		}
		if (PlayerAim)
		{
			PlayerAim->SetAiming(true);
		}
	});
	// Fase 3 · Dash: uno en el piso (la captura sale a medio dash, con estela) y uno en el aire.
	Add(TEXT("dash"), 0.25f, [this]
	{
		if (PlayerAim)
		{
			PlayerAim->SetAiming(false);
		}
		const bool bOk = PlayerDash && PlayerDash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("salto"), 0.7f, [Player] { if (ACharacter* Character = Player()) { Character->Jump(); } });
	Add(TEXT("dash_aire"), 0.8f, [this, Player]
	{
		const bool bFalling = Player() && Player()->GetCharacterMovement()->IsFalling();
		const bool bOk = PlayerDash && PlayerDash->TryDash();
		const bool bSecond = PlayerDash && PlayerDash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash_aire=%d en_aire=%d segundo=%d"), bOk ? 1 : 0, bFalling ? 1 : 0, bSecond ? 1 : 0);
	});
	Add(TEXT("disparo"), 0.9f, [this] { OnFire(); });
	Add(TEXT("escombros"), 1.4f, [] {});
	Add(TEXT("invertir_bala"), 2.6f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && PlayerInversion)
		{
			PlayerInversion->BeginInversionOn(Bullet->Recorder);
		}
	});
	Add(TEXT("rearmado"), 1.6f, [this] { if (PlayerInversion) { PlayerInversion->EndInversion(); } });
	// Fase 3 · Detener: la bala se queda congelada a medio vuelo (después de romper la caja, antes del muro).
	Add(TEXT("disparo_para_detener"), 0.3f, [this] { OnFire(); });
	Add(TEXT("detener_bala"), 2.f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && PlayerTimeStop)
		{
			PlayerTimeStop->StopActor(Bullet);
		}
	});
	Add(TEXT("detener_suelta"), 1.2f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && PlayerTimeStop)
		{
			PlayerTimeStop->ReleaseActor(Bullet);
		}
	});
	Add(TEXT("romper_caja"), 1.4f, [this]
	{
		// Una caja que no esté en la línea de tiro.
		for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
		{
			const ATemporalTurret* Turret = PhysicsLab ? PhysicsLab->GetTurret() : nullptr;
			const FVector From = Turret ? Turret->GetActorLocation() : FVector::ZeroVector;
			const FVector To = Turret ? From + Turret->GetActorForwardVector() * 4000.f : FVector::ZeroVector;
			if (!It->IsBroken() && Turret && !It->SegmentCrosses(From + FVector(0, 0, 60), To + FVector(0, 0, 60)))
			{
				It->Break(It->GetActorLocation() - FVector(0, 0, 30), FVector(0, 0, -1));
				break;
			}
		}
	});
	Add(TEXT("shift_r"), 2.4f, [this] { TourRewindAll(true); });
	Add(TEXT("shift_r_suelta"), 1.f, [this] { TourRewindAll(false); });
	Add(TEXT("sobremarcha"), 2.6f, [this]
	{
		if (PlayerOverdrive)
		{
			PlayerOverdrive->StartOverdrive();
		}
		OnFire();
	});
	Add(TEXT("sobremarcha_off"), 1.2f, [this] { if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); } });
	Add(TEXT("burbuja"), 2.4f, [this, PlaceInLab]
	{
		PlaceInLab(FVector(350.f, -150.f, 120.f));
		OnBubble();
		OnFire();
	});
	// Fase 3 · Sala 5: las torretas te siguen y disparan; morir te regresa a la entrada con la vida llena.
	Add(TEXT("sala5"), 3.f, [this] { OnRoom5(); });
	Add(TEXT("recibir_balas"), 2.5f, [this, Player]
	{
		// Un paso al frente, a la vista de las tres torretas.
		if (ReflexRoom && Player())
		{
			Player()->SetActorLocation(ReflexRoom->RoomToWorld(FVector(700.f, 0.f, 120.f)), false, nullptr, ETeleportType::TeleportPhysics);
		}
	});
	Add(TEXT("muerte_reaparece"), 1.5f, [this]
	{
		const float Before = PlayerHealth ? PlayerHealth->Health : -1.f;
		if (PlayerHealth)
		{
			PlayerHealth->ApplyDamage(PlayerHealth->Health);
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR vida_antes=%.0f"), Before);
	});
	Add(TEXT("reaparecio"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR reaparecio vida=%.0f muerto=%d global=%.2f jugador=%.2f"),
			PlayerHealth ? PlayerHealth->Health : -1.f, PlayerHealth && PlayerHealth->IsDead() ? 1 : 0,
			Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
	});
	// Fase 3 · Esquiva perfecta: una bala a 4 m viene hacia ti y haces dash → tiempo de reflejo, sin daño.
	Add(TEXT("esquiva_perfecta"), 2.f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		const FVector Forward = Character->GetActorForwardVector();
		SpawnTourBullet(Character->GetActorLocation() + Forward * 400.f + FVector(0.f, 0.f, 20.f), -Forward);
		const float Before = PlayerHealth ? PlayerHealth->Health : -1.f;
		const bool bDash = PlayerDash && PlayerDash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR esquiva dash=%d reflex=%d vida=%.0f"), bDash ? 1 : 0, PlayerDash && PlayerDash->IsReflexActive() ? 1 : 0, Before);
	});
	Add(TEXT("despues_del_reflejo"), 1.f, [this]
	{
		ClearTourTemps();
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR despues_del_reflejo vida=%.0f global=%.2f"), PlayerHealth ? PlayerHealth->Health : -1.f, Dilation ? Dilation->GetGlobalDilation() : -1.f);
	});
	// Fase 3 · Ojo lento: apuntando, dos balas pasan a tu lado y se frenan al entrar a 3.5 m.
	Add(TEXT("ojo_lento"), 1.f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		if (PlayerAim)
		{
			PlayerAim->SetAiming(true);
		}
		const FVector Forward = Character->GetActorForwardVector();
		const FVector Right = Character->GetActorRightVector();
		const FVector Chest = Character->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		SpawnTourBullet(Chest + Forward * 600.f + Right * 90.f, -Forward);
		SpawnTourBullet(Chest + Forward * 700.f - Right * 110.f, -Forward);
	});
	Add(TEXT("ojo_lento_revisa"), 1.2f, [this]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_lento frenadas=%d vida=%.0f"), PlayerSlowEye ? PlayerSlowEye->NumSlowed() : -1, PlayerHealth ? PlayerHealth->Health : -1.f);
		if (PlayerAim)
		{
			PlayerAim->SetAiming(false);
		}
	});
	Add(TEXT("ojo_lento_fin"), 0.5f, [this] { ClearTourTemps(); });
	// Fase 3 · Remates: por la espalda (maniquí 0, zona segura) y de frente pero congelado con Detener (maniquí 2).
	auto Mannequin = [this](int32 Index) -> ATemporalMannequin*
	{
		return ReflexRoom && ReflexRoom->GetMannequins().IsValidIndex(Index) ? ReflexRoom->GetMannequins()[Index].Get() : nullptr;
	};
	auto StandAt = [this, Player](const FVector& Where, const FVector& LookAt)
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		Character->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
		const FRotator Facing(0.f, (LookAt - Where).Rotation().Yaw, 0.f);
		Character->SetActorRotation(Facing);
		if (AController* Controller = Character->GetController())
		{
			Controller->SetControlRotation(FRotator(-15.f, Facing.Yaw, 0.f));
		}
	};
	Add(TEXT("remate_acercarse"), 1.f, [this, Mannequin, StandAt]
	{
		OnRoom5();
		if (PlayerHealth) { PlayerHealth->SetInvulnerable(30.f); }
		if (ATemporalMannequin* Target = Mannequin(0))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 150.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("remate_espalda"), 0.4f, [this]
	{
		const bool bShown = PlayerTakedown && PlayerTakedown->GetAvailableTarget();
		const bool bOk = PlayerTakedown && PlayerTakedown->TryTakedown();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_espalda aviso=%d ok=%d"), bShown ? 1 : 0, bOk ? 1 : 0);
	});
	Add(TEXT("maniqui_cae"), 2.f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_cae tirado=%d"), Mannequin(0) && Mannequin(0)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("remate_congelado"), 1.4f, [this, Mannequin, StandAt]
	{
		ATemporalMannequin* Target = Mannequin(2);
		if (!Target)
		{
			return;
		}
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		StandAt(Target->GetActorLocation() + Target->GetActorForwardVector() * 150.f, Target->GetActorLocation());
		const bool bBefore = PlayerTakedown && PlayerTakedown->TryTakedown();
		if (PlayerTimeStop) { PlayerTimeStop->StopActor(Target); }
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_congelado sin_congelar=%d"), bBefore ? 1 : 0);
	});
	Add(TEXT("remate_congelado_golpe"), 1.2f, [this, Mannequin]
	{
		const bool bOk = PlayerTakedown && PlayerTakedown->TryTakedown();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_congelado ok=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("remate_congelado_guardado"), 1.f, [this, Mannequin]
	{
		ATemporalMannequin* Target = Mannequin(2);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR golpe_guardado=%d tirado=%d"), Target && Target->HasPendingHit() ? 1 : 0, Target && Target->IsKnockedDown() ? 1 : 0);
		if (PlayerTimeStop && Target) { PlayerTimeStop->ReleaseActor(Target); }
	});
	Add(TEXT("remate_congelado_sale"), 1.2f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR al_soltar tirado=%d"), Mannequin(2) && Mannequin(2)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("maniqui_se_levanta"), 3.f, [] {});
	Add(TEXT("maniqui_de_pie"), 0.5f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_de_pie m0_tirado=%d m2_tirado=%d"),
			Mannequin(0) && Mannequin(0)->IsKnockedDown() ? 1 : 0, Mannequin(2) && Mannequin(2)->IsKnockedDown() ? 1 : 0);
		if (PlayerHealth) { PlayerHealth->ClearInvulnerable(); }
	});
	// Revisión final · Detener con la mira de verdad: apuntando (Ojo lento) a una bala que viene hacia ti y clic izquierdo.
	Add(TEXT("detener_mira_prepara"), 0.5f, [this]
	{
		OnRoom5();
		if (PlayerHealth) { PlayerHealth->SetInvulnerable(30.f); }
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		if (PlayerAim) { PlayerAim->SetAiming(true); }
	});
	// La bala viene justo por la línea de la mira (como cuando la sigues con el ratón); detrás quedan la torreta y
	// los maniquíes, que antes se robaban el clic.
	Add(TEXT("detener_mira_bala"), 0.25f, [this]
	{
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
			const FVector Forward = CameraRotation.Vector();
			SpawnTourBullet(CameraLocation + Forward * 700.f, -Forward);
		}
	});
	Add(TEXT("detener_mira_clic"), 1.f, [this]
	{
		if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0); PC && TourTemps.Num() > 0 && TourTemps.Last().IsValid())
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
			const FVector ToBullet = TourTemps.Last()->GetActorLocation() - CameraLocation;
			const APawn* Pawn = PC->GetPawn();
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR detener_mira_diag dist_jugador=%.0f angulo=%.1f ritmo=%.2f"),
				Pawn ? FVector::Dist(Pawn->GetActorLocation(), TourTemps.Last()->GetActorLocation()) : -1.f,
				FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ToBullet.GetSafeNormal(), CameraRotation.Vector()))),
				TourTemps.Last()->CustomTimeDilation);
		}
		OnPrimaryPressed();
		AActor* Bullet = TourTemps.Num() > 0 ? TourTemps.Last().Get() : nullptr;
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const AActor* Hovered = PlayerAim ? PlayerAim->GetHoveredTarget() : nullptr;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR detener_con_mira detenida=%d congelada=%d seleccion=%s"),
			Bullet && PlayerTimeStop && PlayerTimeStop->IsStoppedByMe(Bullet) ? 1 : 0, Bullet && Dilation && Dilation->IsFrozen(Bullet) ? 1 : 0,
			Hovered ? *Hovered->GetClass()->GetName() : TEXT("nada"));
	});
	Add(TEXT("detener_mira_fin"), 0.3f, [this]
	{
		if (PlayerTimeStop) { PlayerTimeStop->ReleaseAll(); }
		if (PlayerAim) { PlayerAim->SetAiming(false); }
		ClearTourTemps();
	});
	// Revisión final · Dash hacia la espalda de un maniquí y rematar a medio dash (con doble clic).
	Add(TEXT("dash_remate_prepara"), 0.3f, [this, Mannequin, StandAt]
	{
		if (ATemporalMannequin* Target = Mannequin(0))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 260.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("dash_hacia_maniqui"), 0.1f, [this]
	{
		const bool bOk = PlayerDash && PlayerDash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash_hacia_maniqui dash=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("remate_a_medio_dash"), 0.8f, [this]
	{
		OnPrimaryPressed();
		const bool bBusy = PlayerTakedown && PlayerTakedown->IsBusy();
		const bool bStillDashing = PlayerDash && PlayerDash->IsDashing();
		OnPrimaryPressed();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_en_dash remate=%d sigue_dash=%d detenidos_tras_doble_clic=%d"),
			bBusy ? 1 : 0, bStillDashing ? 1 : 0, PlayerTimeStop ? PlayerTimeStop->NumStopped() : -1);
	});
	Add(TEXT("remate_en_dash_fin"), 0.3f, [this]
	{
		if (PlayerTimeStop) { PlayerTimeStop->ReleaseAll(); }
		if (PlayerHealth) { PlayerHealth->ClearInvulnerable(); }
	});
	Add(TEXT("sala2"), 1.6f, [this, Player]
	{
		GoToRoom(1);
		// Mirar hacia abajo, al vacío: así la captura muestra las plataformas.
		if (ACharacter* Character = Player())
		{
			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(FRotator(-28.f, Controller->GetControlRotation().Yaw, 0.f));
			}
		}
	});
	Add(TEXT("plataforma_cae"), 2.2f, [this]
	{
		if (Chamber && Chamber->GetPlatforms().Num() > 0 && IsValid(Chamber->GetPlatforms()[0]))
		{
			Chamber->GetPlatforms()[0]->Crumble();
		}
	});
	Add(TEXT("invertir_plataforma"), 3.f, [this]
	{
		if (Chamber && Chamber->GetPlatforms().Num() > 0 && IsValid(Chamber->GetPlatforms()[0]) && PlayerInversion)
		{
			PlayerInversion->BeginInversionOn(Chamber->GetPlatforms()[0]->Recorder);
		}
	});
	Add(TEXT("sala1_correr"), 1.4f, [this, Player]
	{
		if (PlayerInversion)
		{
			PlayerInversion->EndInversion();
		}
		GoToRoom(0);
		if (ACharacter* Character = Player())
		{
			Character->LaunchCharacter(FVector(700.f, 250.f, 450.f), true, true);
		}
	});
	Add(TEXT("paso_inverso"), 1.8f, [this] { if (PlayerInverseStep) { PlayerInverseStep->BeginInverseStep(); } });
	Add(TEXT("eco"), 2.2f, [this]
	{
		if (PlayerInverseStep)
		{
			PlayerInverseStep->EndInverseStep();
		}
		OnEcho();
	});

	TourIndex = 0;
	SetActorTickEnabled(true);
	BeginTourStep();
}

ATemporalProjectile* ATemporalSandbox::SpawnTourBullet(const FVector& From, const FVector& Direction)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATemporalProjectile* Bullet = GetWorld()->SpawnActor<ATemporalProjectile>(ATemporalProjectile::StaticClass(), From, FRotator::ZeroRotator, Params);
	if (Bullet)
	{
		Bullet->Speed = 1800.f;
		Bullet->bHurtsPlayer = true;
		Bullet->Launch(From, Direction, nullptr);
		TourTemps.Add(Bullet);
	}
	return Bullet;
}

void ATemporalSandbox::ClearTourTemps()
{
	for (const TWeakObjectPtr<AActor>& Temp : TourTemps)
	{
		if (Temp.IsValid())
		{
			Temp->Destroy();
		}
	}
	TourTemps.Reset();
}

void ATemporalSandbox::BeginTourStep()
{
	TourStepStart = FPlatformTime::Seconds();
	bTourShotTaken = false;
	TourFrames = 0;
	TourFrameTime = 0.0;
	TourMinFps = TNumericLimits<float>::Max();
	// El primer frame de cada paso suele cargar cosas nuevas (teletransporte, efectos): no cuenta para el mínimo.
	TourSkipMinFrames = 2;
	if (TourSteps.IsValidIndex(TourIndex) && TourSteps[TourIndex].Action)
	{
		TourSteps[TourIndex].Action();
	}
}

void ATemporalSandbox::FinishTourStep()
{
	const FEntropiaTourStep& Step = TourSteps[TourIndex];
	const float AvgFps = TourFrameTime > 0.0 ? static_cast<float>(TourFrames / TourFrameTime) : 0.f;
	// Para detectar fugas: cuántos Registros y actores quedan vivos tras cada paso.
	int32 NumActors = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		++NumActors;
	}
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	const int32 NumRecorders = Subsystem ? Subsystem->GetRecorders().Num() : -1;
	const float EnergyLeft = PlayerEnergy ? PlayerEnergy->Energy : -1.f;
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR step=%02d name=%s fps_avg=%.1f fps_min=%.1f actors=%d recorders=%d energy=%.0f"),
		TourIndex, *Step.Name, AvgFps, TourMinFps == TNumericLimits<float>::Max() ? 0.f : TourMinFps, NumActors, NumRecorders, EnergyLeft);

	++TourIndex;
	if (TourSteps.IsValidIndex(TourIndex))
	{
		BeginTourStep();
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR done"));
	SetActorTickEnabled(false);
	TourIndex = INDEX_NONE;
	FGenericPlatformMisc::RequestExit(false);
}

void ATemporalSandbox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!TourSteps.IsValidIndex(TourIndex))
	{
		return;
	}

	// FPS con el tiempo real del frame (sin tope ni dilatación).
	const double FrameTime = FApp::GetDeltaTime();
	if (FrameTime > 0.0)
	{
		++TourFrames;
		TourFrameTime += FrameTime;
		// Los frames de la captura de pantalla son lentos por sí solos: no cuentan para el mínimo.
		if (TourSkipMinFrames > 0)
		{
			--TourSkipMinFrames;
		}
		else
		{
			TourMinFps = FMath::Min(TourMinFps, static_cast<float>(1.0 / FrameTime));
		}
	}

	const FEntropiaTourStep& Step = TourSteps[TourIndex];
	const double Elapsed = FPlatformTime::Seconds() - TourStepStart;
	if (!bTourShotTaken && Elapsed >= Step.Seconds * 0.6)
	{
		bTourShotTaken = true;
		const FString File = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tour"), FString::Printf(TEXT("Tour_%02d_%s.png"), TourIndex, *Step.Name));
		FScreenshotRequest::RequestScreenshot(File, /*bShowUI=*/true, /*bAddFilenameSuffix=*/false);
		TourSkipMinFrames = 3;
	}
	if (Elapsed >= Step.Seconds)
	{
		FinishTourStep();
	}
}

// ---------------------------------------------------------------------------
// Tour "caos": hacer cosas raras a propósito para buscar errores, fugas y estados rotos.

void ATemporalSandbox::BuildChaosSteps()
{
	auto Add = [this](const TCHAR* Name, float Seconds, TFunction<void()> Action)
	{
		TourSteps.Add({ Name, MoveTemp(Action), Seconds });
	};
	auto Player = [this]() { return Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); };
	auto BreakAll = [this]()
	{
		for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
		{
			It->Break(It->GetActorLocation() + FMath::VRand() * 30.f, FMath::VRand());
		}
	};
	auto LookAt = [this, Player](float Pitch)
	{
		if (ACharacter* Character = Player())
		{
			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(FRotator(Pitch, Controller->GetControlRotation().Yaw, 0.f));
			}
		}
	};

	Add(TEXT("sala4"), 1.5f, [this] { OnRoom4(); });
	Add(TEXT("spam_z_x10"), 1.5f, [this] { for (int32 i = 0; i < 10; ++i) { OnFire(); } });
	Add(TEXT("spam_x_x10"), 1.5f, [this] { for (int32 i = 0; i < 10; ++i) { OnFireInverted(); } });
	Add(TEXT("romper_todo_x3"), 1.5f, [BreakAll] { BreakAll(); BreakAll(); BreakAll(); });
	Add(TEXT("spam_b_x10"), 1.f, [this] { for (int32 i = 0; i < 10; ++i) { OnBreakInFront(); } });
	// Girar la cámara y presionar R en frames distintos (como al jugar): la cámara se actualiza al frame siguiente.
	Add(TEXT("mirar_cielo"), 0.6f, [LookAt] { LookAt(70.f); });
	Add(TEXT("r_al_cielo"), 1.f, [this] { if (PlayerInversion) { PlayerInversion->BeginInversion(); } });
	Add(TEXT("r_suelta_y_mira"), 0.6f, [this, LookAt]
	{
		if (PlayerInversion) { PlayerInversion->EndInversion(); }
		LookAt(-15.f);
	});
	// Fase 3: apuntar y soltar muchas veces, e invertir mientras apuntas (la cámara y el giro deben quedar bien).
	Add(TEXT("apuntar_x11_y_r"), 1.f, [this]
	{
		for (int32 i = 0; i < 11; ++i) { if (PlayerAim) { PlayerAim->SetAiming(!PlayerAim->IsAiming()); } }
		if (PlayerInversion) { PlayerInversion->BeginInversion(); }
	});
	Add(TEXT("suelta_apuntar"), 0.6f, [this]
	{
		if (PlayerInversion) { PlayerInversion->EndInversion(); }
		if (PlayerAim) { PlayerAim->SetAiming(false); }
	});
	Add(TEXT("shiftr_y_romper"), 2.f, [this, BreakAll] { TourRewindAll(true); BreakAll(); OnFire(); });
	Add(TEXT("shiftr_suelta"), 0.8f, [this] { TourRewindAll(false); });
	Add(TEXT("r_y_teletransporte"), 1.2f, [this]
	{
		TourRewindAll(true);
		OnRoom4();
		OnRoom4();
		TourRewindAll(false);
	});
	Add(TEXT("v_toggle_x11"), 1.2f, [this] { for (int32 i = 0; i < 11; ++i) { OnOverdrive(); } });
	Add(TEXT("v_f_r_juntos"), 1.5f, [this]
	{
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		if (PlayerInverseStep) { PlayerInverseStep->BeginInverseStep(); }
		if (PlayerInversion) { PlayerInversion->BeginInversion(); }
		OnFire();
	});
	Add(TEXT("soltar_todo"), 0.8f, [this]
	{
		if (PlayerInverseStep) { PlayerInverseStep->EndInverseStep(); }
		if (PlayerInversion) { PlayerInversion->EndInversion(); }
		if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); }
	});
	Add(TEXT("spam_q_x8"), 2.5f, [this] { for (int32 i = 0; i < 8; ++i) { OnBubble(); } OnFire(); });
	Add(TEXT("sin_energia_f_e_v"), 1.f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = 0.f; }
		if (PlayerInverseStep) { PlayerInverseStep->BeginInverseStep(); }
		OnEcho();
		OnOverdrive();
		OnBubble();
	});
	Add(TEXT("spam_e_x6_g"), 1.5f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		for (int32 i = 0; i < 6; ++i) { OnEcho(); }
		OnClearEchoes();
		OnEcho();
	});
	Add(TEXT("teletransportes_x5"), 1.2f, [this] { OnRoom1(); OnRoom2(); OnRoom3(); OnRoom4(); OnBackToStart(); OnRoom4(); });
	Add(TEXT("x_y_r_a_la_bala"), 2.f, [this]
	{
		OnFireInverted();
		OnFire();
		ATemporalProjectile* Newest = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			if (!Newest || It->GetGameTimeSinceCreation() < Newest->GetGameTimeSinceCreation())
			{
				Newest = *It;
			}
		}
		if (Newest && PlayerInversion) { PlayerInversion->BeginInversionOn(Newest->Recorder); }
	});
	Add(TEXT("rebobinar_largo"), 4.f, [this]
	{
		if (PlayerInversion) { PlayerInversion->EndInversion(); }
		TourRewindAll(true);
	});
	Add(TEXT("fin_rebobinar"), 1.f, [this] { TourRewindAll(false); });
	Add(TEXT("churn_romper_rearmar"), 2.5f, [this, BreakAll]
	{
		for (int32 i = 0; i < 4; ++i)
		{
			BreakAll();
			for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It) { It->Restore(); }
		}
		BreakAll();
	});
	Add(TEXT("t_cajas_x5"), 1.5f, [this] { for (int32 i = 0; i < 5; ++i) { OnRespawnCubes(); } });
	// Fase 3 · Detener: más de 3 a la vez, cosas que desaparecen detenidas, invertir algo detenido, burbuja encima.
	Add(TEXT("detener_x6_y_borrar"), 1.5f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		OnRoom4();
		for (int32 i = 0; i < 3; ++i) { OnFire(); }
		int32 Count = 0;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It && Count < 6; ++It, ++Count)
		{
			if (PlayerTimeStop) { PlayerTimeStop->StopActor(*It); }
		}
		for (AActor* Cube : Cubes)
		{
			if (PlayerTimeStop && IsValid(Cube)) { PlayerTimeStop->StopActor(Cube); }
		}
		if (PhysicsLab && PhysicsLab->GetTurret()) { PhysicsLab->GetTurret()->ClearBullets(); }
	});
	Add(TEXT("detener_y_r"), 1.5f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		OnFire();
		ATemporalProjectile* Newest = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It) { Newest = *It; }
		if (Newest && PlayerTimeStop) { PlayerTimeStop->StopActor(Newest); }
		if (Newest && PlayerInversion) { PlayerInversion->BeginInversionOn(Newest->Recorder); }
		OnBubble();
	});
	Add(TEXT("detener_suelta_todo"), 1.f, [this]
	{
		if (PlayerInversion) { PlayerInversion->EndInversion(); }
		if (PlayerTimeStop) { PlayerTimeStop->ReleaseAll(); }
	});
	// Fase 3 · Dash: muchos seguidos, con Sobremarcha, y el Paso Inverso a medio dash.
	Add(TEXT("spam_dash_x10"), 1.f, [this, Player]
	{
		if (Player()) { Player()->Jump(); }
		for (int32 i = 0; i < 10; ++i) { OnDash(); }
	});
	Add(TEXT("dash_v_y_f"), 1.2f, [this]
	{
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		OnDash();
		if (PlayerInverseStep) { PlayerInverseStep->BeginInverseStep(); }
	});
	Add(TEXT("dash_suelta"), 0.8f, [this]
	{
		if (PlayerInverseStep) { PlayerInverseStep->EndInverseStep(); }
		if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); }
		OnDash();
	});
	// Fase 3 · Todos los tiempos lentos encimados y luego soltar todo: el mundo y el jugador deben volver a 1.
	Add(TEXT("todo_lento_junto"), 1.5f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		if (PlayerDash) { PlayerDash->StartReflex(); }
		OnBubble();
		for (int32 i = 0; i < 3; ++i) { OnFire(); }
	});
	Add(TEXT("todo_lento_suelta"), 2.f, [this]
	{
		if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); }
	});
	Add(TEXT("todo_lento_revisa"), 0.5f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR todo_lento global=%.2f jugador=%.2f"),
			Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
	});
	// Fase 3 · Ojo lento + burbuja: al dejar de apuntar, las balas siguen congeladas por la burbuja (gana la más lenta).
	Add(TEXT("ojo_lento_y_burbuja"), 0.3f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		if (PlayerAim) { PlayerAim->SetAiming(true); }
		const FVector Forward = Character->GetActorForwardVector();
		const FVector Right = Character->GetActorRightVector();
		const FVector Chest = Character->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		SpawnTourBullet(Chest + Forward * 250.f - Right * 150.f, Right);
		SpawnTourBullet(Chest + Forward * 300.f - Right * 200.f, Right);
	});
	Add(TEXT("ojo_lento_burbuja"), 0.6f, [this] { OnBubble(); });
	Add(TEXT("ojo_lento_suelta"), 0.6f, [this] { if (PlayerAim) { PlayerAim->SetAiming(false); } });
	Add(TEXT("ojo_lento_revisa_burbuja"), 0.5f, [this]
	{
		float Fastest = -1.f;
		for (const TWeakObjectPtr<AActor>& Temp : TourTemps)
		{
			if (Temp.IsValid()) { Fastest = FMath::Max(Fastest, Temp->CustomTimeDilation); }
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_lento_burbuja bala_mas_rapida=%.2f (0 = sigue congelada)"), Fastest);
		ClearTourTemps();
	});
	// Fase 3 · Morir a media acción: F, R, Sobremarcha, apuntar y dash a la vez.
	Add(TEXT("sala5_caos"), 1.f, [this] { OnRoom5(); });
	Add(TEXT("morir_en_todo"), 1.2f, [this]
	{
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		if (PlayerAim) { PlayerAim->SetAiming(true); }
		if (PlayerInverseStep) { PlayerInverseStep->BeginInverseStep(); }
		if (PlayerInversion) { PlayerInversion->BeginInversion(); }
		if (PlayerHealth) { PlayerHealth->ApplyDamage(1000.f); }
		OnDash();
	});
	Add(TEXT("despues_de_morir"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR despues_de_morir vida=%.0f global=%.2f jugador=%.2f apuntando=%d"),
			PlayerHealth ? PlayerHealth->Health : -1.f, Dilation ? Dilation->GetGlobalDilation() : -1.f,
			Pawn ? Pawn->CustomTimeDilation : -1.f, PlayerAim && PlayerAim->IsAiming() ? 1 : 0);
	});
	// Fase 3 · Remates raros: muchos clics, dash y Sobremarcha a la vez, y teletransportarse a medio remate.
	Add(TEXT("remate_spam"), 1.f, [this]
	{
		OnRoom5();
		if (PlayerHealth) { PlayerHealth->SetInvulnerable(30.f); }
		if (ReflexRoom && ReflexRoom->GetMannequins().Num() > 0)
		{
			if (ATemporalMannequin* Target = ReflexRoom->GetMannequins()[0])
			{
				if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
				{
					Pawn->SetActorLocation(Target->GetActorLocation() - Target->GetActorForwardVector() * 120.f, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
	});
	Add(TEXT("remate_x5_dash_v"), 1.f, [this]
	{
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		for (int32 i = 0; i < 5; ++i) { OnPrimaryPressed(); OnDash(); }
	});
	Add(TEXT("remate_y_teletransporte"), 1.f, [this]
	{
		if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); }
		OnPrimaryPressed();
		OnRoom4();
	});
	// Revisión final · Ojo lento en Sobremarcha: la bala va a 0.25 (gana la más lenta), no a 0.25 × 0.25.
	Add(TEXT("ojo_en_sobremarcha"), 0.3f, [this]
	{
		OnRoom5();
		if (PlayerHealth) { PlayerHealth->SetInvulnerable(30.f); }
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		if (PlayerOverdrive) { PlayerOverdrive->StartOverdrive(); }
		if (PlayerAim) { PlayerAim->SetAiming(true); }
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector Forward = Pawn->GetActorForwardVector();
			const FVector Right = Pawn->GetActorRightVector();
			SpawnTourBullet(Pawn->GetActorLocation() + FVector(0.f, 0.f, 20.f) + Forward * 250.f - Right * 150.f, Right);
		}
	});
	Add(TEXT("ojo_en_sobremarcha_revisa"), 0.3f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const AActor* Bullet = TourTemps.Num() > 0 ? TourTemps.Last().Get() : nullptr;
		const float Rate = Bullet && Dilation ? Bullet->CustomTimeDilation * Dilation->GetGlobalDilation() : -1.f;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_en_sobremarcha ritmo_bala=%.3f (esperado 0.250)"), Rate);
		if (PlayerOverdrive) { PlayerOverdrive->StopOverdrive(); }
		if (PlayerAim) { PlayerAim->SetAiming(false); }
		ClearTourTemps();
	});
	// Revisión final · Detener a un maniquí que va cayendo: el cuerpo debe quedarse quieto.
	auto StandBehind = [this](int32 Index, float Distance)
	{
		ATemporalMannequin* Target = ReflexRoom && ReflexRoom->GetMannequins().IsValidIndex(Index) ? ReflexRoom->GetMannequins()[Index].Get() : nullptr;
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Target && Pawn)
		{
			Pawn->SetActorLocation(Target->GetActorLocation() - Target->GetActorForwardVector() * Distance, false, nullptr, ETeleportType::TeleportPhysics);
		}
		return Target;
	};
	Add(TEXT("remate_para_detener_cayendo"), 0.35f, [this, StandBehind]
	{
		OnRoom5();
		if (PlayerHealth) { PlayerHealth->SetInvulnerable(30.f); }
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		StandBehind(0, 120.f);
		OnPrimaryPressed();
	});
	Add(TEXT("detener_cayendo"), 0.5f, [this]
	{
		ATemporalMannequin* Target = ReflexRoom && ReflexRoom->GetMannequins().Num() > 0 ? ReflexRoom->GetMannequins()[0].Get() : nullptr;
		if (Target && PlayerTimeStop)
		{
			PlayerTimeStop->StopActor(Target);
			TourMark = Target->GetMesh()->GetBoneLocation(TEXT("pelvis"));
		}
	});
	Add(TEXT("detener_cayendo_revisa"), 0.3f, [this]
	{
		ATemporalMannequin* Target = ReflexRoom && ReflexRoom->GetMannequins().Num() > 0 ? ReflexRoom->GetMannequins()[0].Get() : nullptr;
		const float Moved = Target ? FVector::Dist(TourMark, Target->GetMesh()->GetBoneLocation(TEXT("pelvis"))) : -1.f;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_cayendo_detenido se_movio=%.1f cm tirado=%d"), Moved, Target && Target->IsKnockedDown() ? 1 : 0);
		if (PlayerTimeStop) { PlayerTimeStop->ReleaseAll(); }
	});
	// Revisión final · Morir con todo activo (dash, reflejo, algo detenido, remate): al reaparecer no queda nada.
	Add(TEXT("morir_con_todo_activo"), 0.2f, [this, StandBehind]
	{
		OnRoom5();
		if (PlayerEnergy) { PlayerEnergy->Energy = PlayerEnergy->MaxEnergy; }
		StandBehind(0, 120.f);
		OnDash();
		if (PlayerDash) { PlayerDash->StartReflex(); }
		if (PlayerTimeStop && ReflexRoom && ReflexRoom->GetTurrets().Num() > 0) { PlayerTimeStop->StopActor(ReflexRoom->GetTurrets()[0]); }
		OnPrimaryPressed();
		if (PlayerHealth)
		{
			PlayerHealth->ClearInvulnerable();
			PlayerHealth->ApplyDamage(1000.f);
		}
	});
	Add(TEXT("morir_con_todo_revisa"), 0.5f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tras_morir_con_todo global=%.2f detenidos=%d dash=%d reflejo=%d remate=%d"),
			Dilation ? Dilation->GetGlobalDilation() : -1.f, PlayerTimeStop ? PlayerTimeStop->NumStopped() : -1,
			PlayerDash && PlayerDash->IsDashing() ? 1 : 0, PlayerDash && PlayerDash->IsReflexActive() ? 1 : 0,
			PlayerTakedown && PlayerTakedown->IsBusy() ? 1 : 0);
	});
	Add(TEXT("remate_revisa"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_revisa global=%.2f jugador=%.2f"), Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
		if (PlayerHealth) { PlayerHealth->ClearInvulnerable(); }
	});
	Add(TEXT("calma"), 2.5f, [] {});
}
