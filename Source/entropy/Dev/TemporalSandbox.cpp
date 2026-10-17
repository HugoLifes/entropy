// ENTROPÍA · Caja de arena temporal

#include "TemporalSandbox.h"
#include "EntropiaControlsComponent.h"
#include "EntropiaPlayerPowers.h"
#include "EntropiaRoomsComponent.h"
#include "EntropiaTourComponent.h"
#include "TemporalHealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATemporalSandbox::ATemporalSandbox()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Powers = CreateDefaultSubobject<UEntropiaPlayerPowers>(TEXT("Powers"));
	Rooms = CreateDefaultSubobject<UEntropiaRoomsComponent>(TEXT("Rooms"));
	Controls = CreateDefaultSubobject<UEntropiaControlsComponent>(TEXT("Controls"));
	Tour = CreateDefaultSubobject<UEntropiaTourComponent>(TEXT("Tour"));
}

void ATemporalSandbox::BeginPlay()
{
	Super::BeginPlay();
	// Esperamos un frame para que el jugador ya exista.
	GetWorldTimerManager().SetTimerForNextTick(this, &ATemporalSandbox::Setup);
}

void ATemporalSandbox::Setup()
{
	// 1) El jugador y sus poderes (va primero: las salas copian su cuerpo para los maniquíes).
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Powers->EquipPlayer(Pawn);
		if (Powers->Health)
		{
			Powers->Health->OnDeath.AddUObject(this, &ATemporalSandbox::OnPlayerDied);
		}
	}

	// 2) Teclas.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		Controls->BindKeys(InputComponent, Powers, Rooms);
	}

	// 3) Cajas y salas.
	Rooms->BuildAll(Powers);
	Controls->ShowHelp();

	// 4) Modo tour.
	if (UEntropiaTourComponent::IsRequested())
	{
		Tour->Start(Powers, Rooms);
	}
}

void ATemporalSandbox::OnPlayerDied()
{
	// Al siguiente frame: no reaparecer en medio del golpe de la bala.
	GetWorldTimerManager().SetTimerForNextTick(this, &ATemporalSandbox::RespawnAfterDeath);
}

void ATemporalSandbox::RespawnAfterDeath()
{
	Powers->CancelAll();
	Rooms->RespawnPlayer();
}
