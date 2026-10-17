// ENTROPÍA · Brecha

#include "TimeBubble.h"
#include "TemporalFX.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

ATimeBubble::ATimeBubble()
{
	// Tick solo para aplicar la gravedad escalada a los cuerpos en cámara lenta.
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

	TemporalFX::ApplyToAllSlots(Visual, TemporalFX::MakeTranslucent(this, VisualColor));
	Visual->SetWorldScale3D(FVector(0.01f));
	TemporalFX::Burst(this, GetActorLocation(), FLinearColor(VisualColor.R, VisualColor.G, VisualColor.B), 1.2f);

	// Lo que ya estaba dentro cuando apareció la burbuja.
	TArray<AActor*> Inside;
	Sphere->GetOverlappingActors(Inside);
	for (AActor* Actor : Inside)
	{
		Affect(Actor);
	}
	// La vida se cuenta en Tick con tiempo real (no SetLifeSpan): con la Sobremarcha el mundo va lento,
	// pero tus poderes duran lo mismo de siempre.
}

void ATimeBubble::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Al desaparecer, todo vuelve al tiempo normal.
	TArray<TWeakObjectPtr<AActor>> ToRelease = Affected.Array();
	for (const TWeakObjectPtr<AActor>& Actor : ToRelease)
	{
		Release(Actor.Get());
	}
	Affected.Reset();
	FrozenBodies.Reset();
	SlowedBodies.Reset();

	Super::EndPlay(EndPlayReason);
}

void ATimeBubble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Visual: crece rápido al aparecer y se encoge en los últimos 0.3 s. La esfera básica mide 100 cm.
	Age += TemporalFX::RealDelta();
	if (LifeSeconds > 0.f && Age >= LifeSeconds)
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

	// En cámara lenta todo pasa Dilation veces más lento: la velocidad se escala por Dilation
	// y la aceleración (gravedad) por Dilation². Es una aproximación: los choques siguen siendo a velocidad real.
	const float GravityZ = GetWorld()->GetGravityZ() * Dilation * Dilation;
	for (const TPair<TWeakObjectPtr<AActor>, FSlowedBody>& Pair : SlowedBodies)
	{
		AActor* Actor = Pair.Key.Get();
		UPrimitiveComponent* Root = Actor ? Cast<UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr;
		if (Root && Root->IsSimulatingPhysics() && Pair.Value.bHadGravity)
		{
			Root->AddForce(FVector(0.f, 0.f, GravityZ), NAME_None, /*bAccelChange=*/true);
		}
	}
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
	if (!bAffectPlayer && Actor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}

	Affected.Add(Actor);
	Actor->CustomTimeDilation = Dilation;

	// La física de Unreal no respeta CustomTimeDilation, así que congelamos los cuerpos a mano.
	UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (Root && Root->IsSimulatingPhysics() && Dilation < 0.05f)
	{
		FFrozenBody Body;
		Body.LinearVelocity = Root->GetPhysicsLinearVelocity();
		Body.AngularVelocity = Root->GetPhysicsAngularVelocityInDegrees();
		FrozenBodies.Add(Actor, Body);
		Root->SetSimulatePhysics(false);
	}
	else if (Root && Root->IsSimulatingPhysics() && !FMath::IsNearlyEqual(Dilation, 1.f))
	{
		FSlowedBody Body;
		Body.bHadGravity = Root->IsGravityEnabled();
		SlowedBodies.Add(Actor, Body);
		Root->SetEnableGravity(false);
		Root->SetPhysicsLinearVelocity(Root->GetPhysicsLinearVelocity() * Dilation);
		Root->SetPhysicsAngularVelocityInDegrees(Root->GetPhysicsAngularVelocityInDegrees() * Dilation);
	}
}

void ATimeBubble::Release(AActor* Actor)
{
	if (!Actor || !Affected.Contains(Actor))
	{
		return;
	}

	Affected.Remove(Actor);
	Actor->CustomTimeDilation = 1.f;

	if (const FFrozenBody* Body = FrozenBodies.Find(Actor))
	{
		if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
		{
			Root->SetSimulatePhysics(true);
			Root->SetPhysicsLinearVelocity(Body->LinearVelocity);
			Root->SetPhysicsAngularVelocityInDegrees(Body->AngularVelocity);
		}
		FrozenBodies.Remove(Actor);
	}
	else if (const FSlowedBody* Slowed = SlowedBodies.Find(Actor))
	{
		if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
		{
			Root->SetEnableGravity(Slowed->bHadGravity);
			// Si el Registro Temporal lo está rebobinando, la física está apagada: no tocamos la velocidad.
			if (Root->IsSimulatingPhysics())
			{
				Root->SetPhysicsLinearVelocity(Root->GetPhysicsLinearVelocity() / Dilation);
				Root->SetPhysicsAngularVelocityInDegrees(Root->GetPhysicsAngularVelocityInDegrees() / Dilation);
			}
		}
		SlowedBodies.Remove(Actor);
	}
}
