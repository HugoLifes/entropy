// ENTROPÍA · Cuarto de pruebas

#include "TemporalTestChamber.h"
#include "CrumblePlatform.h"
#include "InverseStepComponent.h"
#include "MovingHazard.h"
#include "PressurePlateDoor.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Medidas del cuarto (cm, coordenadas locales).
	constexpr float HalfWidth = 925.f;
	constexpr float WallHeight = 450.f;
	constexpr float Thick = 50.f;
	constexpr float RoomBStart = 2000.f;
	constexpr float PitStart = 2600.f;
	constexpr float PitEnd = 3900.f;
	constexpr float RoomCStart = 4500.f;
	constexpr float ChamberEnd = 7200.f;
	constexpr float PitFloorZ = -800.f;
	constexpr float KillZ = -650.f;
	constexpr float GoalX = 6900.f;

	const FLinearColor FloorColor(0.18f, 0.18f, 0.2f);
	const FLinearColor EcoColor(0.25f, 0.45f, 0.9f);
	const FLinearColor InversoColor(0.9f, 0.55f, 0.15f);
	const FLinearColor BrechaColor(0.2f, 0.85f, 0.75f);
}

ATemporalTestChamber::ATemporalTestChamber()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATemporalTestChamber::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ATemporalTestChamber::AddBlock(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision)
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
	// Sin eventos de traslape: así la burbuja de Brecha no congela al cuarto entero.
	Block->SetGenerateOverlapEvents(false);
	Block->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
	Block->RegisterComponent();
	AddInstanceComponent(Block);
	TemporalFX::ApplyToAllSlots(Block, TemporalFX::MakeSolid(this, Color));
}

void ATemporalTestChamber::Build()
{
	const float WallZ = WallHeight * 0.5f;

	// --- Pisos ---
	AddBlock(FVector(RoomBStart * 0.5f, 0.f, -Thick * 0.5f), FVector(RoomBStart, HalfWidth * 2.f, Thick), FloorColor);
	AddBlock(FVector((RoomBStart + PitStart) * 0.5f, 0.f, -Thick * 0.5f), FVector(PitStart - RoomBStart, HalfWidth * 2.f, Thick), FloorColor);
	AddBlock(FVector((PitEnd + RoomCStart) * 0.5f, 0.f, -Thick * 0.5f), FVector(RoomCStart - PitEnd, HalfWidth * 2.f, Thick), FloorColor);
	AddBlock(FVector((RoomCStart + ChamberEnd) * 0.5f, 0.f, -Thick * 0.5f), FVector(ChamberEnd - RoomCStart, HalfWidth * 2.f, Thick), FloorColor);
	// Fondo del vacío (ahí caen las plataformas para poder rebobinarlas).
	AddBlock(FVector((PitStart + PitEnd) * 0.5f, 0.f, PitFloorZ - Thick * 0.5f), FVector(PitEnd - PitStart, HalfWidth * 2.f, Thick), FLinearColor(0.05f, 0.05f, 0.06f));

	// --- Paredes exteriores (llegan hasta el fondo del vacío) ---
	const float OuterBottom = PitFloorZ - Thick;
	const float OuterHeight = WallHeight - OuterBottom;
	const float OuterZ = OuterBottom + OuterHeight * 0.5f;
	for (const float Side : { -1.f, 1.f })
	{
		AddBlock(FVector(ChamberEnd * 0.5f, Side * (HalfWidth + Thick * 0.5f), OuterZ), FVector(ChamberEnd, Thick, OuterHeight), FloorColor * 1.4f);
	}
	AddBlock(FVector(-Thick * 0.5f, 0.f, OuterZ), FVector(Thick, HalfWidth * 2.f + Thick * 2.f, OuterHeight), FloorColor * 1.4f);
	AddBlock(FVector(ChamberEnd + Thick * 0.5f, 0.f, OuterZ), FVector(Thick, HalfWidth * 2.f + Thick * 2.f, OuterHeight), FloorColor * 1.4f);

	// --- Sala 1 · ECO: muro con hueco de 300 cm que tapa la puerta de la placa ---
	{
		const float Gap = 150.f;
		const float PieceWidth = HalfWidth - Gap;
		for (const float Side : { -1.f, 1.f })
		{
			AddBlock(FVector(RoomBStart, Side * (Gap + PieceWidth * 0.5f), WallZ), FVector(Thick, PieceWidth, WallHeight), EcoColor);
		}
		// Dintel sobre la puerta (para que no se pueda saltar por arriba).
		AddBlock(FVector(RoomBStart, 0.f, 300.f + (WallHeight - 300.f) * 0.5f), FVector(Thick, Gap * 2.f, WallHeight - 300.f), EcoColor);

		// Placa en (1000, 600). La puerta (40 x 300 x 300) queda justo en el hueco del muro.
		const FVector PlateLocal(1000.f, 600.f, 60.f);
		const FVector DoorLocal(RoomBStart, 0.f, 150.f);
		const FTransform PlateTransform(GetActorRotation(), GetActorTransform().TransformPosition(PlateLocal));
		PlateDoor = GetWorld()->SpawnActorDeferred<APressurePlateDoor>(APressurePlateDoor::StaticClass(), PlateTransform, this);
		if (PlateDoor)
		{
			PlateDoor->DoorOffset = DoorLocal - PlateLocal;
			UGameplayStatics::FinishSpawningActor(PlateDoor, PlateTransform);
		}
	}

	// --- Sala 2 · INVERSO: 4 plataformas sobre el vacío ---
	{
		const float Gap = 200.f;
		const float PieceWidth = HalfWidth - Gap;
		for (const float Side : { -1.f, 1.f })
		{
			AddBlock(FVector(RoomCStart, Side * (Gap + PieceWidth * 0.5f), WallZ), FVector(Thick, PieceWidth, WallHeight), InversoColor);
		}
		// Bordes naranjas del vacío para que se vea dónde empieza.
		AddBlock(FVector(PitStart - 10.f, 0.f, 2.f), FVector(20.f, HalfWidth * 2.f, 4.f), InversoColor, false);
		AddBlock(FVector(PitEnd + 10.f, 0.f, 2.f), FVector(20.f, HalfWidth * 2.f, 4.f), InversoColor, false);

		const float PlatformXs[] = { 2725.f, 3045.f, 3365.f, 3685.f };
		for (const float X : PlatformXs)
		{
			const FTransform T(GetActorRotation(), GetActorTransform().TransformPosition(FVector(X, 0.f, -25.f)));
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ACrumblePlatform* Platform = GetWorld()->SpawnActor<ACrumblePlatform>(ACrumblePlatform::StaticClass(), T, Params))
			{
				Platforms.Add(Platform);
			}
		}
	}

	// --- Sala 3 · BRECHA: muro con la prensa como única salida ---
	{
		const float HazardX = 5800.f;
		const float Gap = 250.f;
		const float PieceWidth = HalfWidth - Gap;
		for (const float Side : { -1.f, 1.f })
		{
			AddBlock(FVector(HazardX, Side * (Gap + PieceWidth * 0.5f), WallZ), FVector(300.f, PieceWidth, WallHeight), BrechaColor);
		}

		const FTransform T(GetActorRotation(), GetActorTransform().TransformPosition(FVector(HazardX, 0.f, 150.f)));
		Hazard = GetWorld()->SpawnActorDeferred<AMovingHazard>(AMovingHazard::StaticClass(), T, this);
		if (Hazard)
		{
			Hazard->Size = FVector(300.f, 500.f, 300.f);
			Hazard->Lift = 450.f;
			Hazard->RespawnPoint = GetActorTransform().TransformPosition(RoomStart(2));
			UGameplayStatics::FinishSpawningActor(Hazard, T);
		}

		// Meta: tapete verde al final.
		AddBlock(FVector(GoalX + 150.f, 0.f, 2.f), FVector(300.f, 600.f, 4.f), FLinearColor(0.1f, 1.f, 0.3f), false);
	}
}

FVector ATemporalTestChamber::RoomStart(int32 RoomIndex) const
{
	switch (RoomIndex)
	{
	case 1: return FVector(RoomBStart + 200.f, 0.f, 120.f);
	case 2: return FVector(RoomCStart + 200.f, 0.f, 120.f);
	default: return FVector(200.f, 0.f, 120.f);
	}
}

int32 ATemporalTestChamber::RoomAt(float LocalX) const
{
	return LocalX < RoomBStart ? 0 : (LocalX < RoomCStart ? 1 : 2);
}

void ATemporalTestChamber::TeleportToRoom(int32 RoomIndex)
{
	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return;
	}

	RoomIndex = FMath::Clamp(RoomIndex, 0, 2);
	if (UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>())
	{
		Inverse->EndInverseStep();
	}

	const FVector Target = GetActorTransform().TransformPosition(RoomStart(RoomIndex));
	Character->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	}
	// El pasado de antes del teletransporte no sirve aquí.
	if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
	{
		Recorder->ClearRecording();
	}

	if (RoomIndex == 1)
	{
		ResetFallenPlatforms(true);
	}
	bGoalReached = false;
	CurrentRoom = RoomIndex;
	ShowRoomHint(RoomIndex);
	TemporalFX::Burst(this, Target, FLinearColor(0.4f, 0.8f, 1.f), 0.8f);
}

void ATemporalTestChamber::ResetFallenPlatforms(bool bAlsoAnchored)
{
	for (ACrumblePlatform* Platform : Platforms)
	{
		if (IsValid(Platform) && (bAlsoAnchored || !Platform->IsAnchored()))
		{
			Platform->ResetPlatform();
		}
	}
}

void ATemporalTestChamber::ShowRoomHint(int32 RoomIndex)
{
	if (!GEngine)
	{
		return;
	}

	static const TCHAR* Hints[] = {
		TEXT("SALA 1 · ECO: parate ~3 s en la placa roja, camina hasta la puerta azul y pulsa E. Tu eco pisa la placa y te abre."),
		TEXT("SALA 2 · INVERSO: las plataformas caen al pisarlas. APUNTA a una caida y MANTEN R: si vuelve a su lugar queda ANCLADA (azul). Una por una. Si caes, MANTEN F."),
		TEXT("SALA 3 · BRECHA: la prensa roja aplasta. Cuando este ARRIBA pulsa Q frente a ella para congelarla y pasa por debajo."),
	};
	const FColor Colors[] = { FColor(90, 140, 255), FColor(255, 160, 60), FColor(60, 230, 200) };
	GEngine->AddOnScreenDebugMessage(1003, 20.f, Colors[RoomIndex], Hints[RoomIndex]);
}

void ATemporalTestChamber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return;
	}

	const FVector Local = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
	const bool bInside = Local.X > -Thick && Local.X < ChamberEnd + Thick && FMath::Abs(Local.Y) < HalfWidth + Thick && Local.Z > PitFloorZ - 200.f && Local.Z < 2000.f;
	if (!bInside)
	{
		CurrentRoom = INDEX_NONE;
		return;
	}

	const int32 Room = RoomAt(Local.X);
	if (Room != CurrentRoom)
	{
		CurrentRoom = Room;
		ShowRoomHint(Room);
	}

	// Caíste al vacío: regresas al inicio de la sala (salvo que estés rebobinando con F).
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	if (Local.Z < KillZ && !(Inverse && Inverse->IsInverseActive()))
	{
		TemporalFX::Burst(this, Character->GetActorLocation(), FLinearColor(1.f, 0.4f, 0.1f));
		const FVector Target = GetActorTransform().TransformPosition(RoomStart(Room));
		Character->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->StopMovementImmediately();
		if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
		{
			Recorder->ClearRecording();
		}
		// Las plataformas caídas vuelven a su lugar; las ancladas se quedan.
		ResetFallenPlatforms(false);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3002, 2.5f, FColor::Orange, TEXT("Caiste. Tip: manten F mientras caes para regresar en el tiempo."));
		}
		return;
	}

	if (!bGoalReached && Local.X > GoalX && Local.Z > -100.f)
	{
		bGoalReached = true;
		TemporalFX::Burst(this, Character->GetActorLocation(), FLinearColor(0.1f, 1.f, 0.3f), 2.f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3003, 8.f, FColor::Green, TEXT("¡CUARTO DE PRUEBAS SUPERADO! (1/2/3 para repetir una sala)"));
		}
	}
}
