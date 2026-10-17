// ENTROPÍA · Modo tour

#include "EntropiaTourComponent.h"
#include "EntropiaPlayerPowers.h"
#include "EntropiaRoomsComponent.h"
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

UEntropiaTourComponent::UEntropiaTourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UEntropiaTourComponent::IsRequested()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EntropiaTour"));
}

void UEntropiaTourComponent::Start(UEntropiaPlayerPowers* InPowers, UEntropiaRoomsComponent* InRooms)
{
	Powers = InPowers;
	Rooms = InRooms;
	FString Mode;
	FParse::Value(FCommandLine::Get(), TEXT("EntropiaTour="), Mode);
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR start mode=%s"), Mode.IsEmpty() ? TEXT("normal") : *Mode);
	if (Mode.Equals(TEXT("caos"), ESearchCase::IgnoreCase))
	{
		BuildChaosSteps();
	}
	else
	{
		BuildNormalSteps();
	}
	TourIndex = 0;
	SetComponentTickEnabled(true);
	BeginTourStep();
}

ATemporalProjectile* UEntropiaTourComponent::SpawnTourBullet(const FVector& From, const FVector& Direction)
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

ATemporalProjectile* UEntropiaTourComponent::SpawnPlayerTourBullet(const FVector& From, const FVector& Direction, float TorsoDamage, float HeadDamage, float Impulse)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATemporalProjectile* Bullet = GetWorld()->SpawnActor<ATemporalProjectile>(ATemporalProjectile::StaticClass(), From, FRotator::ZeroRotator, Params);
	if (Bullet)
	{
		Bullet->Speed = 8000.f;
		Bullet->bFromPlayer = true;
		Bullet->TorsoDamage = TorsoDamage;
		Bullet->HeadDamage = HeadDamage;
		Bullet->Impulse = Impulse;
		Bullet->Launch(From, Direction, UGameplayStatics::GetPlayerPawn(this, 0));
		TourTemps.Add(Bullet);
	}
	return Bullet;
}

void UEntropiaTourComponent::ClearTourTemps()
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

void UEntropiaTourComponent::BeginTourStep()
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

void UEntropiaTourComponent::FinishTourStep()
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
	const float EnergyLeft = Powers->Energy ? Powers->Energy->Energy : -1.f;
	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR step=%02d name=%s fps_avg=%.1f fps_min=%.1f actors=%d recorders=%d energy=%.0f"),
		TourIndex, *Step.Name, AvgFps, TourMinFps == TNumericLimits<float>::Max() ? 0.f : TourMinFps, NumActors, NumRecorders, EnergyLeft);

	++TourIndex;
	if (TourSteps.IsValidIndex(TourIndex))
	{
		BeginTourStep();
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR done"));
	SetComponentTickEnabled(false);
	TourIndex = INDEX_NONE;
	FGenericPlatformMisc::RequestExit(false);
}

void UEntropiaTourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
