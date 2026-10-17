// ENTROPÍA · Placa de presión + puerta

#include "PressurePlateDoor.h"
#include "EchoGhost.h"
#include "TemporalFX.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APressurePlateDoor::APressurePlateDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(100.f, 100.f, 60.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(Trigger);
	PlateMesh->SetRelativeLocation(FVector(0.f, 0.f, -55.f));
	PlateMesh->SetRelativeScale3D(FVector(2.f, 2.f, 0.1f));
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Trigger);
	DoorMesh->SetRelativeScale3D(FVector(0.4f, 3.f, 3.f));
	DoorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	DoorMesh->SetMobility(EComponentMobility::Movable);

	if (Cube.Succeeded())
	{
		PlateMesh->SetStaticMesh(Cube.Object);
		DoorMesh->SetStaticMesh(Cube.Object);
	}
}

void APressurePlateDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DoorMesh->SetRelativeLocation(DoorOffset);
}

void APressurePlateDoor::BeginPlay()
{
	Super::BeginPlay();
	DoorMesh->SetRelativeLocation(DoorOffset);

	// El material básico del motor tiene el parámetro "Color": rojo = libre, verde = presionada.
	PlateMaterial = PlateMesh->CreateDynamicMaterialInstance(0);
	if (UMaterialInstanceDynamic* DoorMaterial = DoorMesh->CreateDynamicMaterialInstance(0))
	{
		DoorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.35f, 1.f));
	}
}

void APressurePlateDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> Inside;
	Trigger->GetOverlappingActors(Inside);
	const bool bWasPressed = bPressed;
	bPressed = false;
	for (const AActor* Actor : Inside)
	{
		if (Actor && (Actor->IsA<APawn>() || Actor->IsA<AEchoGhost>()))
		{
			bPressed = true;
			break;
		}
	}

	if (bPressed && !bWasPressed)
	{
		TemporalFX::Burst(this, PlateMesh->GetComponentLocation(), FLinearColor(0.1f, 1.f, 0.2f), 0.6f);
	}

	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, bPressed ? 1.f : 0.f, DeltaTime, OpenSpeed);
	DoorMesh->SetRelativeLocation(DoorOffset + FVector(0.f, 0.f, OpenHeight * OpenAlpha));

	if (PlateMaterial)
	{
		PlateMaterial->SetVectorParameterValue(TEXT("Color"), bPressed ? FLinearColor(0.1f, 1.f, 0.2f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}
