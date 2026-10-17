// ENTROPÍA · Sala 4 · Física temporal

#include "TemporalPhysicsLab.h"
#include "InverseStepComponent.h"
#include "TemporalBreakable.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalTurret.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace EntropiaLab
{
	// Medidas (cm, coordenadas locales). Namespace con nombre: en los "unity builds" varios .cpp se compilan juntos
	// y unos nombres genéricos chocarían con los del cuarto de pruebas o del motor. La torreta dispara hacia +X a lo largo de Y = 0.
	constexpr float Length = 2400.f;
	constexpr float HalfWidth = 800.f;
	constexpr float WallHeight = 450.f;
	constexpr float Thick = 50.f;

	const FLinearColor FloorColor(0.18f, 0.18f, 0.2f);
	const FLinearColor WallColor(0.25f, 0.25f, 0.28f);
	const FLinearColor LabColor(0.85f, 0.45f, 0.15f);
	const FLinearColor CrateColor(0.55f, 0.38f, 0.2f);
	const FLinearColor PanelColor(0.7f, 0.7f, 0.75f);

	const FVector StartLocal(300.f, 450.f, 120.f);
}

ATemporalPhysicsLab::ATemporalPhysicsLab()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalPhysicsLab::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATemporalPhysicsLab::AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision)
{
	static TWeakObjectPtr<UStaticMesh> CubeMesh;
	if (!CubeMesh.IsValid())
	{
		CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	if (!CubeMesh.IsValid())
	{
		return;
	}

	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetStaticMesh(CubeMesh.Get());
	Block->SetupAttachment(GetRootComponent());
	Block->SetRelativeLocation(Center);
	Block->SetRelativeScale3D(Size / 100.f);
	// Sin traslape (la burbuja no congela la sala). BlockAll = WorldStatic: la bala invertida lo toma como pared.
	Block->SetGenerateOverlapEvents(false);
	Block->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
	Block->RegisterComponent();
	AddInstanceComponent(Block);
	TemporalFX::ApplyToAllSlots(Block, TemporalFX::MakeSolid(this, Color));
}

ATemporalBreakable* ATemporalPhysicsLab::AddBreakable(const FVector& Center, const FVector& Size, const FIntVector& Pieces, const FLinearColor& Color)
{
	const FTransform T(GetActorRotation(), GetActorTransform().TransformPosition(Center));
	ATemporalBreakable* Breakable = GetWorld()->SpawnActorDeferred<ATemporalBreakable>(ATemporalBreakable::StaticClass(), T, this);
	if (Breakable)
	{
		Breakable->Size = Size;
		Breakable->Pieces = Pieces;
		Breakable->Color = Color;
		UGameplayStatics::FinishSpawningActor(Breakable, T);
		Breakables.Add(Breakable);
	}
	return Breakable;
}

void ATemporalPhysicsLab::Build()
{
	const float WallZ = EntropiaLab::WallHeight * 0.5f;

	// Piso, paredes laterales y pared trasera (detrás de la torreta).
	AddBlock(FVector(EntropiaLab::Length * 0.5f, 0.f, -EntropiaLab::Thick * 0.5f), FVector(EntropiaLab::Length, EntropiaLab::HalfWidth * 2.f, EntropiaLab::Thick), EntropiaLab::FloorColor);
	for (const float Side : { -1.f, 1.f })
	{
		AddBlock(FVector(EntropiaLab::Length * 0.5f, Side * (EntropiaLab::HalfWidth + EntropiaLab::Thick * 0.5f), WallZ), FVector(EntropiaLab::Length, EntropiaLab::Thick, EntropiaLab::WallHeight), EntropiaLab::WallColor);
	}
	AddBlock(FVector(-EntropiaLab::Thick * 0.5f, 0.f, WallZ), FVector(EntropiaLab::Thick, EntropiaLab::HalfWidth * 2.f + EntropiaLab::Thick * 2.f, EntropiaLab::WallHeight), EntropiaLab::WallColor);

	// Pared de tiro: gruesa y con una franja naranja donde pegan las balas.
	AddBlock(FVector(EntropiaLab::Length + 50.f, 0.f, WallZ), FVector(100.f, EntropiaLab::HalfWidth * 2.f + EntropiaLab::Thick * 2.f, EntropiaLab::WallHeight), EntropiaLab::WallColor);
	AddBlock(FVector(EntropiaLab::Length - 2.f, 0.f, 60.f), FVector(4.f, 160.f, 40.f), EntropiaLab::LabColor, false);

	// Torreta al inicio, apuntando a +X. El cañón queda a 60 cm de altura.
	{
		const FTransform T(GetActorRotation(), GetActorTransform().TransformPosition(FVector(250.f, 0.f, 0.f)));
		FActorSpawnParameters Params;
		Params.Owner = this;
		Turret = GetWorld()->SpawnActor<ATemporalTurret>(ATemporalTurret::StaticClass(), T, Params);
	}

	// En la línea de tiro: una caja (la bala la cruza a 60 cm) y luego un muro.
	AddBreakable(FVector(800.f, 0.f, 50.f), FVector(100.f, 100.f, 100.f), FIntVector(2, 2, 2), EntropiaLab::CrateColor);
	AddBreakable(FVector(1500.f, 0.f, 150.f), FVector(40.f, 500.f, 300.f), FIntVector(1, 5, 3), EntropiaLab::PanelColor);

	// Fuera de la línea: cajas para romper con B (o tirándoles las cajas del Sandbox).
	AddBreakable(FVector(1000.f, -500.f, 50.f), FVector(100.f, 100.f, 100.f), FIntVector(2, 2, 2), EntropiaLab::CrateColor);
	AddBreakable(FVector(1150.f, 500.f, 50.f), FVector(100.f, 100.f, 100.f), FIntVector(2, 2, 2), EntropiaLab::CrateColor);

	// Línea en el piso que marca el camino de las balas.
	AddBlock(FVector(EntropiaLab::Length * 0.5f + 200.f, 0.f, 1.f), FVector(EntropiaLab::Length - 400.f, 8.f, 2.f), EntropiaLab::LabColor, false);
}

void ATemporalPhysicsLab::ResetLab()
{
	for (ATemporalBreakable* Breakable : Breakables)
	{
		if (IsValid(Breakable))
		{
			Breakable->Restore();
		}
	}
	if (Turret)
	{
		Turret->ClearBullets();
	}
}

void ATemporalPhysicsLab::TeleportPlayerHere()
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

	const FVector Target = GetActorTransform().TransformPosition(EntropiaLab::StartLocal);
	Character->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	}
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}

	ResetLab();
	ShowHint();
	TemporalFX::Burst(this, Target, FLinearColor(1.f, 0.6f, 0.2f), 0.8f);
}

void ATemporalPhysicsLab::ShowHint() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1003, 25.f, FColor(255, 150, 60),
			TEXT("SALA 4 · FISICA: Z dispara (rompe caja y muro) · APUNTA a la bala y MANTEN R: vuelve al canon y rearma lo que rompio · Ctrl+R: todo · X: bala INVERTIDA · B: rompe lo que tengas enfrente"));
	}
}

void ATemporalPhysicsLab::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}

	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const bool bInside = Local.X > 0.f && Local.X < EntropiaLab::Length && FMath::Abs(Local.Y) < EntropiaLab::HalfWidth && Local.Z > -200.f && Local.Z < EntropiaLab::WallHeight + 300.f;
	if (bInside && !bPlayerInside)
	{
		ShowHint();
	}
	bPlayerInside = bInside;
}
