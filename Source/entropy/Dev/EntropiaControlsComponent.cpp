// ENTROPÍA · Teclas del banco de pruebas

#include "EntropiaControlsComponent.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaParkourComponent.h"
#include "EntropiaCoverComponent.h"
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

UEntropiaControlsComponent::UEntropiaControlsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEntropiaControlsComponent::BindKeys(UInputComponent* Input, UEntropiaPlayerPowers* InPowers, UEntropiaRoomsComponent* InRooms)
{
	Powers = InPowers;
	Rooms = InRooms;
	if (!Input)
	{
		return;
	}
	Input->BindKey(EKeys::F, IE_Pressed, this, &UEntropiaControlsComponent::OnInversePressed);
	Input->BindKey(EKeys::F, IE_Released, this, &UEntropiaControlsComponent::OnInverseReleased);
	Input->BindKey(EKeys::R, IE_Pressed, this, &UEntropiaControlsComponent::OnRewindPressed);
	Input->BindKey(EKeys::R, IE_Released, this, &UEntropiaControlsComponent::OnRewindReleased);
	Input->BindKey(EKeys::E, IE_Pressed, this, &UEntropiaControlsComponent::OnEcho);
	Input->BindKey(EKeys::G, IE_Pressed, this, &UEntropiaControlsComponent::OnClearEchoes);
	Input->BindKey(EKeys::Q, IE_Pressed, this, &UEntropiaControlsComponent::OnBubble);
	Input->BindKey(EKeys::T, IE_Pressed, this, &UEntropiaControlsComponent::OnRespawnCubes);
	Input->BindKey(EKeys::One, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom1);
	Input->BindKey(EKeys::Two, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom2);
	Input->BindKey(EKeys::Three, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom3);
	Input->BindKey(EKeys::Zero, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom0);
	Input->BindKey(EKeys::Four, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom4);
	Input->BindKey(EKeys::Five, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom5);
	Input->BindKey(EKeys::Six, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom6);
	Input->BindKey(EKeys::Seven, IE_Pressed, this, &UEntropiaControlsComponent::OnRoom7);
	Input->BindKey(EKeys::Z, IE_Pressed, this, &UEntropiaControlsComponent::OnFire);
	Input->BindKey(EKeys::X, IE_Pressed, this, &UEntropiaControlsComponent::OnFireInverted);
	Input->BindKey(EKeys::B, IE_Pressed, this, &UEntropiaControlsComponent::OnBreakInFront);
	Input->BindKey(EKeys::V, IE_Pressed, this, &UEntropiaControlsComponent::OnOverdrive);
	Input->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UEntropiaControlsComponent::OnAimPressed);
	Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &UEntropiaControlsComponent::OnAimReleased);
	Input->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UEntropiaControlsComponent::OnPrimaryPressed);
	Input->BindKey(EKeys::LeftShift, IE_Pressed, this, &UEntropiaControlsComponent::OnDash);
	Input->BindKey(EKeys::LeftShift, IE_Released, this, &UEntropiaControlsComponent::OnSprintReleased);
	Input->BindKey(EKeys::C, IE_Pressed, this, &UEntropiaControlsComponent::OnCrouch);
}

void UEntropiaControlsComponent::ShowHelp() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1000, 40.f, FColor::Cyan,
			TEXT("ENTROPIA · Clic der.: apuntar · Clic izq.: golpe (apuntando: detener) · Shift: dash (mantener: correr) · C: agacharse o cubrirse · F (mantener): Paso Inverso · R (mantener): invertir lo que apuntas · Ctrl+R: todo · V: Sobremarcha · E: eco · G: borrar ecos · Q: burbuja · T: soltar cajas"));
		GEngine->AddOnScreenDebugMessage(1002, 40.f, FColor::Green,
			TEXT("SALAS · 1: Eco · 2: Inverso · 3: Brecha · 4: Fisica (Z dispara · X bala invertida · B romper) · 5: Reflejos · 6: Parkour · 7: Combate · 0: volver a las cajas"));
	}
}

void UEntropiaControlsComponent::OnInversePressed()
{
	if (Powers->InverseStep)
	{
		Powers->InverseStep->BeginInverseStep();
	}
}

void UEntropiaControlsComponent::OnInverseReleased()
{
	if (Powers->InverseStep)
	{
		Powers->InverseStep->EndInverseStep();
	}
}

void UEntropiaControlsComponent::OnRewindPressed()
{
	// Ctrl+R = rebobinar TODO (para pruebas, sin costo). R sola = Invertir objeto: solo lo que apuntas.
	// (Antes era Shift+R; Shift ahora es el dash.)
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	bRewindingAll = PC && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl));
	if (!bRewindingAll)
	{
		if (Powers->Inversion)
		{
			Powers->Inversion->BeginInversion();
		}
		return;
	}

	Rooms->RewindAll(true);
}

void UEntropiaControlsComponent::OnRewindReleased()
{
	if (!bRewindingAll)
	{
		if (Powers->Inversion)
		{
			Powers->Inversion->EndInversion();
		}
		return;
	}

	bRewindingAll = false;
	Rooms->RewindAll(false);
}

void UEntropiaControlsComponent::OnEcho()
{
	Powers->CreateEcho();
}

void UEntropiaControlsComponent::OnClearEchoes()
{
	Powers->ClearEchoes();
}

void UEntropiaControlsComponent::OnBubble()
{
	Powers->SpawnBubble();
}

void UEntropiaControlsComponent::OnRespawnCubes()
{
	Rooms->SpawnCubes();
}

void UEntropiaControlsComponent::OnRoom0()
{
	Rooms->GoToRoom(0);
}

void UEntropiaControlsComponent::OnRoom1()
{
	Rooms->GoToRoom(1);
}

void UEntropiaControlsComponent::OnRoom2()
{
	Rooms->GoToRoom(2);
}

void UEntropiaControlsComponent::OnRoom3()
{
	Rooms->GoToRoom(3);
}

void UEntropiaControlsComponent::OnRoom4()
{
	Rooms->GoToRoom(4);
}

void UEntropiaControlsComponent::OnRoom5()
{
	Rooms->GoToRoom(5);
}

void UEntropiaControlsComponent::OnRoom6()
{
	Rooms->GoToRoom(6);
}

void UEntropiaControlsComponent::OnRoom7()
{
	Rooms->GoToRoom(7);
}

void UEntropiaControlsComponent::OnFire()
{
	Rooms->Fire();
}

void UEntropiaControlsComponent::OnFireInverted()
{
	Rooms->FireInverted();
}

void UEntropiaControlsComponent::OnBreakInFront()
{
	Rooms->BreakInFront();
}

void UEntropiaControlsComponent::OnOverdrive()
{
	Powers->ToggleOverdrive();
}

void UEntropiaControlsComponent::OnAimPressed()
{
	if (Powers->Aim)
	{
		Powers->Aim->SetAiming(true);
	}
}

void UEntropiaControlsComponent::OnAimReleased()
{
	if (Powers->Aim)
	{
		Powers->Aim->SetAiming(false);
	}
}

void UEntropiaControlsComponent::OnPrimaryPressed()
{
	Powers->PrimaryAction();
}

void UEntropiaControlsComponent::OnDash()
{
	// Shift: el dash sale al presionar; si lo mantienes, sigues corriendo.
	Powers->TryDash();
	if (Powers->Locomotion)
	{
		Powers->Locomotion->SetSprint(true);
	}
}

void UEntropiaControlsComponent::OnSprintReleased()
{
	if (Powers->Locomotion)
	{
		Powers->Locomotion->SetSprint(false);
	}
}

void UEntropiaControlsComponent::OnCrouch()
{
	// Colgado de una orilla, C te suelta.
	if (Powers->Parkour && Powers->Parkour->IsHanging())
	{
		Powers->Parkour->Drop();
		return;
	}
	// Cubierto: C te despega. Junto a una pared o barda: te cubres. Si no: te agachas.
	if (Powers->Cover && Powers->Cover->IsInCover())
	{
		Powers->Cover->Exit();
		return;
	}
	if (Powers->Cover && Powers->Cover->TryEnter())
	{
		return;
	}
	if (Powers->Locomotion)
	{
		Powers->Locomotion->ToggleCrouch();
	}
}

