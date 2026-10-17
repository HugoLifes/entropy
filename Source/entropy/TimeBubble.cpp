// ENTROPÍA · Brecha

#include "TimeBubble.h"
#include "TemporalDilation.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

ATimeBubble::ATimeBubble()
{
	// Tick para la vida (en tiempo real), el visual y la distorsión.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->InitSphereRadius(400.f);
	Sphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Sphere->SetGenerateOverlapEvents(true);
	SetRootComponent(Sphere);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Sphere);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCastShadow(false);
	if (SphereMesh.Succeeded())
	{
		Visual->SetStaticMesh(SphereMesh.Object);
	}
}

void ATimeBubble::BeginPlay()
{
	Super::BeginPlay();

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ATimeBubble::OnEnter);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &ATimeBubble::OnExit);

	TemporalFX::ApplyToAllSlots(Visual, TemporalFX::MakeField(this, VisualColor));
	Visual->SetWorldScale3D(FVector(0.01f));
	TemporalFX::Burst(this, GetActorLocation(), FLinearColor(VisualColor.R, VisualColor.G, VisualColor.B), 1.2f);

	// Lo que ya estaba dentro cuando apareció la burbuja (o su único objetivo).
	bHasOnlyTarget = OnlyTarget.IsValid();
	if (bHasOnlyTarget)
	{
		Affect(OnlyTarget.Get());
	}
	else
	{
		TArray<AActor*> Inside;
		Sphere->GetOverlappingActors(Inside);
		for (AActor* Actor : Inside)
		{
			Affect(Actor);
		}
	}
	// La vida se cuenta en Tick con tiempo real (no SetLifeSpan): con la Sobremarcha el mundo va lento,
	// pero tus poderes duran lo mismo de siempre.
}

void ATimeBubble::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Al desaparecer, todo vuelve al tiempo normal (o al de otro efecto que siga encima).
	if (UTemporalDilationSubsystem* DilationSubsystem = UTemporalDilationSubsystem::Get(this))
	{
		DilationSubsystem->ReleaseSource(this);
	}
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->ClearField(this);
	}
	Affected.Reset();

	Super::EndPlay(EndPlayReason);
}

void ATimeBubble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Visual: crece rápido al aparecer y se encoge en los últimos 0.3 s. La esfera básica mide 100 cm.
	Age += TemporalFX::RealDelta();
	if ((LifeSeconds > 0.f && Age >= LifeSeconds) || (bHasOnlyTarget && !OnlyTarget.IsValid()))
	{
		Destroy();
		return;
	}
	float Grow = FMath::Clamp(Age / 0.25f, 0.f, 1.f);
	Grow = 1.f - FMath::Square(1.f - Grow);
	if (LifeSeconds > 0.f)
	{
		Grow *= FMath::Clamp((LifeSeconds - Age) / 0.3f, 0.f, 1.f);
	}
	Visual->SetWorldScale3D(FVector(FMath::Max(0.01f, Sphere->GetScaledSphereRadius() / 50.f * Grow)));

	// Distorsión: lo de adentro se ve congelado (más fuerte cuanto más lento va el tiempo) y el borde ondula.
	UTemporalFXSubsystem* FX = bDrawField ? UTemporalFXSubsystem::Get(this) : nullptr;
	if (FX)
	{
		const float Frozen = FMath::Clamp(1.f - Dilation, 0.f, 1.f);
		FX->SetField(this, GetActorLocation(), Sphere->GetScaledSphereRadius() * Grow, Frozen * Grow);
	}
	// La gravedad escalada de los cuerpos en cámara lenta la aplica el árbitro del tiempo.
}

void ATimeBubble::OnEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Affect(OtherActor);
}

void ATimeBubble::OnExit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Release(OtherActor);
}

void ATimeBubble::Affect(AActor* Actor)
{
	if (!Actor || Actor == this || Affected.Contains(Actor))
	{
		return;
	}
	if (bHasOnlyTarget && Actor != OnlyTarget.Get())
	{
		return;
	}
	if (!bAffectPlayer && Actor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}

	Affected.Add(Actor);
	if (UTemporalDilationSubsystem* DilationSubsystem = UTemporalDilationSubsystem::Get(this))
	{
		DilationSubsystem->Request(Actor, this, Dilation);
	}
}

void ATimeBubble::Release(AActor* Actor)
{
	if (!Actor || !Affected.Contains(Actor))
	{
		return;
	}

	Affected.Remove(Actor);
	if (UTemporalDilationSubsystem* DilationSubsystem = UTemporalDilationSubsystem::Get(this))
	{
		DilationSubsystem->Release(Actor, this);
	}

	// La burbujita de Detener existe solo para su objetivo: si sale (por ejemplo, lo rebobinas con R), se acaba.
	if (bHasOnlyTarget && Actor == OnlyTarget.Get() && !IsActorBeingDestroyed())
	{
		Destroy();
	}
}
