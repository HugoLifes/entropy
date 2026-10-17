// ENTROPÍA · Poderes del jugador

#include "EntropiaPlayerPowers.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaParkourComponent.h"
#include "EntropiaCoverComponent.h"
#include "EntropiaMeleeComponent.h"
#include "EntropiaGrabComponent.h"
#include "TemporalWorldStopComponent.h"
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

UEntropiaPlayerPowers::UEntropiaPlayerPowers()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEntropiaPlayerPowers::EquipPlayer(APawn* Pawn)
{
	// El jugador graba su posición y su pose (la usan el Eco y el Paso Inverso).
	if (Pawn)
	{
		Recorder = Pawn->FindComponentByClass<UTemporalRecorderComponent>();
		if (!Recorder)
		{
			Recorder = NewObject<UTemporalRecorderComponent>(Pawn, TEXT("PlayerTemporalRecorder"));
			Recorder->MaxRecordSeconds = EchoSeconds;
			Recorder->bRecordPose = true;
			Recorder->RegisterComponent();
		}

		// La energía va primero: todos los poderes la buscan al empezar.
		Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(Pawn, TEXT("PlayerTemporalEnergy"));
		Health = TemporalFX::FindOrAddComponent<UTemporalHealthComponent>(Pawn, TEXT("Health"));

		InverseStep = Pawn->FindComponentByClass<UInverseStepComponent>();
		if (!InverseStep)
		{
			InverseStep = NewObject<UInverseStepComponent>(Pawn, TEXT("InverseStep"));
			InverseStep->RegisterComponent();
		}

		// La mira va antes que los poderes que la usan (Invertir, Detener).
		// La locomoción va antes que la mira: la mira guarda el giro "base" que la locomoción ya ajustó.
		Locomotion = TemporalFX::FindOrAddComponent<UEntropiaLocomotionComponent>(Pawn, TEXT("Locomotion"));
		Parkour = TemporalFX::FindOrAddComponent<UEntropiaParkourComponent>(Pawn, TEXT("Parkour"));
		Cover = TemporalFX::FindOrAddComponent<UEntropiaCoverComponent>(Pawn, TEXT("Cover"));
		Melee = TemporalFX::FindOrAddComponent<UEntropiaMeleeComponent>(Pawn, TEXT("Melee"));
		Grab = TemporalFX::FindOrAddComponent<UEntropiaGrabComponent>(Pawn, TEXT("Grab"));
		WorldStop = TemporalFX::FindOrAddComponent<UTemporalWorldStopComponent>(Pawn, TEXT("WorldStop"));
		Aim = TemporalFX::FindOrAddComponent<UTemporalAimComponent>(Pawn, TEXT("Aim"));
		Inversion = TemporalFX::FindOrAddComponent<UObjectInversionComponent>(Pawn, TEXT("PlayerObjectInversion"));
		TimeStop = TemporalFX::FindOrAddComponent<UTimeStopComponent>(Pawn, TEXT("TimeStop"));
		Dash = TemporalFX::FindOrAddComponent<UTemporalDashComponent>(Pawn, TEXT("Dash"));
		SlowEye = TemporalFX::FindOrAddComponent<USlowEyeComponent>(Pawn, TEXT("SlowEye"));
		Takedown = TemporalFX::FindOrAddComponent<UTakedownComponent>(Pawn, TEXT("Takedown"));
		Overdrive = TemporalFX::FindOrAddComponent<UOverdriveComponent>(Pawn, TEXT("Overdrive"));

		// Barra de energía en pantalla.
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			HUD = CreateWidget<UTemporalHUDWidget>(PC, UTemporalHUDWidget::StaticClass());
			if (HUD)
			{
				HUD->SetEnergy(Energy);
				HUD->SetAim(Aim);
				HUD->SetHealth(Health);
				HUD->SetDash(Dash);
				HUD->SetTakedown(Takedown);
				HUD->SetGrab(Grab);
				HUD->AddToViewport();
			}
		}
	}

}

int32 UEntropiaPlayerPowers::NumEchoes() const
{
	return Echoes.Num();
}

void UEntropiaPlayerPowers::CreateEcho()
{
	if (!Recorder || Recorder->IsRewinding() || Recorder->GetRecordedSeconds() < 0.5f)
	{
		return;
	}
	if (Energy && !Energy->Spend(EchoCost))
	{
		Energy->Deny();
		return;
	}

	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const TArray<FTemporalFrame> Frames = Recorder->GetRecording();

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
			FString::Printf(TEXT("Eco %d/%d: repitiendo %.1f s en bucle"), Echoes.Num(), MaxEchoes, Recorder->GetRecordedSeconds()));
	}
}

void UEntropiaPlayerPowers::ClearEchoes()
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

void UEntropiaPlayerPowers::SpawnBubble()
{
	// Nivel 2: el mundo entero se detiene (Star Platinum: The World).
	if (TimeStopLevel >= 2 && WorldStop)
	{
		WorldStop->Start();
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	if (Energy && !Energy->Spend(BubbleCost))
	{
		Energy->Deny();
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

void UEntropiaPlayerPowers::PrimaryAction()
{
	// Si el HUD muestra un remate, el clic remata. Durante un remate los clics extra no hacen nada.
	if (Takedown && (Takedown->IsBusy() || Takedown->TryTakedown()))
	{
		return;
	}
	// Apuntando: Detener lo que está en la mira. Sin apuntar: golpes (combo de 3).
	if (Aim && Aim->IsAiming())
	{
		if (TimeStop)
		{
			TimeStop->StopAimed();
		}
		return;
	}
	if (Melee)
	{
		Melee->TryAttack();
	}
}

void UEntropiaPlayerPowers::TryDash()
{
	if (Dash)
	{
		Dash->TryDash();
	}
}

void UEntropiaPlayerPowers::ToggleOverdrive()
{
	if (Overdrive)
	{
		Overdrive->Toggle();
	}
}

void UEntropiaPlayerPowers::CycleTimeStopLevel()
{
	TimeStopLevel = TimeStopLevel >= 2 ? 1 : 2;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1008, 3.f, FColor(200, 160, 255),
			TimeStopLevel >= 2 ? TEXT("Detener el tiempo: NIVEL 2 (Q detiene todo el mundo)") : TEXT("Detener el tiempo: NIVEL 1 (Q = burbuja)"));
	}
}

void UEntropiaPlayerPowers::EndForTeleport()
{
	// Teletransportarte o morir corta el mundo detenido (nunca se queda congelado ni sin color).
	if (WorldStop)
	{
		WorldStop->Stop();
	}
	if (Parkour)
	{
		Parkour->Cancel();
	}
	if (Cover)
	{
		Cover->Exit();
	}
	if (Grab && Grab->IsHolding())
	{
		Grab->Release(false);
	}
	if (InverseStep)
	{
		InverseStep->EndInverseStep();
	}
	if (Inversion)
	{
		Inversion->EndInversion();
	}
}

void UEntropiaPlayerPowers::CancelAll()
{
	// Morir suelta todo: F, R, Sobremarcha, apuntar, lo detenido, el dash, el tiempo de reflejo y el remate
	// (nada queda pegado ni en cámara lenta al reaparecer).
	EndForTeleport();
	if (Overdrive)
	{
		Overdrive->StopOverdrive();
	}
	if (TimeStop)
	{
		TimeStop->ReleaseAll();
	}
	if (Dash)
	{
		Dash->CancelDash();
		Dash->CancelReflex();
	}
	if (Takedown)
	{
		Takedown->Cancel();
	}
	if (Aim)
	{
		Aim->SetAiming(false);
	}
	ClearEchoes();
}
