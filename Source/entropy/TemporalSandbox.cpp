// ENTROPÍA · Caja de arena temporal

#include "TemporalSandbox.h"
#include "TemporalFXSubsystem.h"
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
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR start"));
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
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR step=%02d name=%s fps_avg=%.1f fps_min=%.1f"),
		TourIndex, *Step.Name, AvgFps, TourMinFps == TNumericLimits<float>::Max() ? 0.f : TourMinFps);

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
