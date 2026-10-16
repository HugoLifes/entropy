// ENTROPÍA · Plataforma que se derrumba

#include "CrumblePlatform.h"
#include "TemporalRecorderComponent.h"
#include "TemporalFX.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACrumblePlatform::ACrumblePlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	SetRootComponent(Mesh);

	// Detector sobre la cara superior (en espacio local del cubo de 100 cm).
	StepTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StepTrigger"));
	StepTrigger->SetupAttachment(Mesh);
	StepTrigger->InitBoxExtent(FVector(50.f, 50.f, 50.f));
	StepTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	StepTrigger->SetGenerateOverlapEvents(true);

	Recorder = CreateDefaultSubobject<UTemporalRecorderComponent>(TEXT("Recorder"));
}

void ACrumblePlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetWorldScale3D(Size / 100.f);
	// El trigger hereda la escala: lo subimos una "unidad" para que quede encima de la plataforma.
	StepTrigger->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
}

void ACrumblePlatform::BeginPlay()
{
	Super::BeginPlay();
	Mesh->SetWorldScale3D(Size / 100.f);
	HomeTransform = GetActorTransform();
	Mesh->SetSimulatePhysics(false);
	Recorder->OnRewindFinished.AddDynamic(this, &ACrumblePlatform::HandleRewindFinished);

	Material = TemporalFX::MakeSolid(this, FLinearColor(0.55f, 0.35f, 0.2f));
	TemporalFX::ApplyToAllSlots(Mesh, Material);
}

void ACrumblePlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAnchored || bCrumbled || Recorder->IsRewinding())
	{
		return;
	}

	if (!bWarning)
	{
		TArray<AActor*> OnTop;
		StepTrigger->GetOverlappingActors(OnTop, APawn::StaticClass());
		if (OnTop.Num() > 0)
		{
			bWarning = true;
			WarningTime = 0.f;
		}
		return;
	}

	// Tiembla antes de caer.
	WarningTime += DeltaTime;
	const float Shake = FMath::Sin(WarningTime * 80.f) * 4.f;
	SetActorLocation(HomeTransform.GetLocation() + FVector(Shake, 0.f, 0.f));
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.3f, 0.1f));
	}

	if (WarningTime >= CrumbleDelay)
	{
		Crumble();
	}
}

void ACrumblePlatform::Crumble()
{
	bCrumbled = true;
	bWarning = false;
	SetActorTransform(HomeTransform);
	Mesh->SetSimulatePhysics(true);
	Mesh->AddImpulse(FVector(0.f, 0.f, -200.f), NAME_None, true);
	TemporalFX::Burst(this, GetActorLocation(), FLinearColor(1.f, 0.5f, 0.2f), 0.6f);
}

void ACrumblePlatform::HandleRewindFinished()
{
	// Si cayó y el rebobinado la dejó en su sitio, queda anclada en el tiempo.
	if (bCrumbled && FVector::Dist(GetActorLocation(), HomeTransform.GetLocation()) < 30.f)
	{
		ResetPlatform();
		bAnchored = true;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.5f, 1.f));
		}
		TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.3f, 0.7f, 1.f), 0.6f);
	}
}

void ACrumblePlatform::ResetPlatform()
{
	Mesh->SetSimulatePhysics(false);
	SetActorTransform(HomeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	bCrumbled = false;
	bAnchored = false;
	bWarning = false;
	WarningTime = 0.f;
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.35f, 0.2f));
	}
}
