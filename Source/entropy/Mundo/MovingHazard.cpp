// ENTROPÍA · Prensa del tiempo

#include "MovingHazard.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AMovingHazard::AMovingHazard()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// Necesario para que la burbuja de Brecha la detecte y la congele.
	Mesh->SetGenerateOverlapEvents(true);
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	SetRootComponent(Mesh);

	// Un poco más angosta que la prensa (no mata al tocarla de lado) y más alta (detecta el aplastón).
	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(Mesh);
	HitBox->InitBoxExtent(FVector(48.f, 48.f, 55.f));
	HitBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AMovingHazard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetWorldScale3D(Size / 100.f);
}

void AMovingHazard::BeginPlay()
{
	Super::BeginPlay();
	Mesh->SetWorldScale3D(Size / 100.f);
	RestLocation = GetActorLocation();
	TemporalFX::ApplyToAllSlots(Mesh, TemporalFX::MakeSolid(this, FLinearColor(0.8f, 0.1f, 0.1f)));
}

void AMovingHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Cooldown -= DeltaTime;

	// Ciclo: sube rápido, se queda arriba un instante y baja de golpe.
	Time = FMath::Fmod(Time + DeltaTime, Period);
	const float Phase = Time / Period;
	const float RiseEnd = 0.25f;
	const float UpEnd = RiseEnd + UpFraction;
	float Alpha = 0.f;
	if (Phase < RiseEnd)
	{
		Alpha = FMath::SmoothStep(0.f, RiseEnd, Phase);
	}
	else if (Phase < UpEnd)
	{
		Alpha = 1.f;
	}
	else
	{
		Alpha = 1.f - FMath::Clamp((Phase - UpEnd) / 0.08f, 0.f, 1.f);
	}
	SetActorLocation(RestLocation + FVector(0.f, 0.f, Lift * Alpha));

	if (Cooldown > 0.f)
	{
		return;
	}

	TArray<AActor*> Hit;
	HitBox->GetOverlappingActors(Hit, ACharacter::StaticClass());
	for (AActor* Actor : Hit)
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!Character || !Character->IsPlayerControlled())
		{
			continue;
		}
		Cooldown = 0.5f;
		TemporalFX::Burst(this, Character->GetActorLocation(), FLinearColor(1.f, 0.1f, 0.1f));
		Character->SetActorLocation(RespawnPoint, false, nullptr, ETeleportType::TeleportPhysics);
		Character->GetCharacterMovement()->StopMovementImmediately();
		if (UTemporalRecorderComponent* Recorder = Character->FindComponentByClass<UTemporalRecorderComponent>())
		{
			Recorder->ClearRecording();
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3001, 2.f, FColor::Red, TEXT("¡Aplastado! Congela la prensa con Q mientras esta arriba."));
		}
		break;
	}
}
