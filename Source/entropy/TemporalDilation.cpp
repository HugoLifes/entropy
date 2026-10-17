// ENTROPÍA · Árbitro del tiempo

#include "TemporalDilation.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Libro de pedidos (lógica pura)

float FTemporalDilationLedger::Slowest(const TMap<const void*, float>& BySource)
{
	float Result = 1.f;
	bool bAny = false;
	for (const TPair<const void*, float>& Pair : BySource)
	{
		Result = bAny ? FMath::Min(Result, Pair.Value) : Pair.Value;
		bAny = true;
	}
	return bAny ? Result : 1.f;
}

float FTemporalDilationLedger::Get(const void* Key) const
{
	const TMap<const void*, float>* BySource = Requests.Find(Key);
	return BySource ? Slowest(*BySource) : 1.f;
}

bool FTemporalDilationLedger::Request(const void* Key, const void* Source, float Dilation)
{
	const float Before = Get(Key);
	Requests.FindOrAdd(Key).Add(Source, FMath::Max(Dilation, 0.f));
	return !FMath::IsNearlyEqual(Before, Get(Key));
}

bool FTemporalDilationLedger::Release(const void* Key, const void* Source)
{
	TMap<const void*, float>* BySource = Requests.Find(Key);
	if (!BySource || !BySource->Contains(Source))
	{
		return false;
	}
	const float Before = Slowest(*BySource);
	BySource->Remove(Source);
	const float After = Slowest(*BySource);
	if (BySource->Num() == 0)
	{
		Requests.Remove(Key);
	}
	return !FMath::IsNearlyEqual(Before, After);
}

TArray<const void*> FTemporalDilationLedger::ReleaseSource(const void* Source)
{
	TArray<const void*> Keys;
	Requests.GetKeys(Keys);
	TArray<const void*> Changed;
	for (const void* Key : Keys)
	{
		if (Release(Key, Source))
		{
			Changed.Add(Key);
		}
	}
	return Changed;
}

// ---------------------------------------------------------------------------
// Subsistema

UTemporalDilationSubsystem* UTemporalDilationSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTemporalDilationSubsystem>() : nullptr;
}

bool UTemporalDilationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTemporalDilationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTemporalDilationSubsystem, STATGROUP_Tickables);
}

float UTemporalDilationSubsystem::PlayerTimeFor(float OwnDilation, float GlobalDilation)
{
	return OwnDilation / FMath::Max(GlobalDilation, 0.0001f);
}

APawn* UTemporalDilationSubsystem::GetPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

void UTemporalDilationSubsystem::ForgetIfStale(AActor* Actor)
{
	const TWeakObjectPtr<AActor>* Known = KeyActors.Find(Actor);
	if (Known && Known->Get() != Actor)
	{
		Actors.RemoveKey(Actor);
		KeyActors.Remove(Actor);
	}
}

void UTemporalDilationSubsystem::Request(AActor* Actor, const void* Source, float Dilation)
{
	if (!IsValid(Actor))
	{
		return;
	}
	ForgetIfStale(Actor);
	KeyActors.Add(Actor, Actor);
	const float Before = Actors.Get(Actor);
	if (Actors.Request(Actor, Source, Dilation))
	{
		Apply(Actor, Before, Actors.Get(Actor));
	}
}

void UTemporalDilationSubsystem::Release(AActor* Actor, const void* Source)
{
	if (!Actor)
	{
		return;
	}
	ForgetIfStale(Actor);
	const float Before = Actors.Get(Actor);
	const bool bChanged = Actors.Release(Actor, Source);
	if (!Actors.Has(Actor))
	{
		KeyActors.Remove(Actor);
	}
	if (bChanged && IsValid(Actor))
	{
		Apply(Actor, Before, Actors.Get(Actor));
	}
}

void UTemporalDilationSubsystem::ReleaseSource(const void* Source)
{
	// Guardamos las velocidades de antes para saber cuánto escalar la física de cada uno.
	TArray<const void*> Keys;
	Actors.GetKeys(Keys);
	TMap<const void*, float> Before;
	for (const void* Key : Keys)
	{
		Before.Add(Key, Actors.Get(Key));
	}

	for (const void* Key : Actors.ReleaseSource(Source))
	{
		const TWeakObjectPtr<AActor>* Weak = KeyActors.Find(Key);
		AActor* Actor = Weak ? Weak->Get() : nullptr;
		if (!Actors.Has(Key))
		{
			KeyActors.Remove(Key);
		}
		if (Actor == Key && IsValid(Actor))
		{
			Apply(Actor, Before.FindRef(Key), Actors.Get(Key));
		}
	}
	ReleaseGlobal(Source);
}

float UTemporalDilationSubsystem::GetDilation(const AActor* Actor) const
{
	return Actor ? Actors.Get(Actor) : 1.f;
}

void UTemporalDilationSubsystem::RequestGlobal(const void* Source, float WorldDilation)
{
	if (Global.Request(this, Source, FMath::Clamp(WorldDilation, 0.01f, 1.f)))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, GetGlobalDilation());
		ApplyPlayer();
	}
}

void UTemporalDilationSubsystem::ReleaseGlobal(const void* Source)
{
	if (Global.Release(this, Source))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, GetGlobalDilation());
		ApplyPlayer();
	}
}

void UTemporalDilationSubsystem::ApplyPlayer()
{
	if (APawn* Player = GetPlayer())
	{
		Player->CustomTimeDilation = PlayerTimeFor(Actors.Get(Player), GetGlobalDilation());
	}
}

void UTemporalDilationSubsystem::Apply(AActor* Actor, float OldDilation, float NewDilation)
{
	if (Actor == GetPlayer())
	{
		ApplyPlayer();
	}
	else
	{
		Actor->CustomTimeDilation = NewDilation;
	}
	ApplyBody(Actor, OldDilation, NewDilation);
}

void UTemporalDilationSubsystem::ApplyBody(AActor* Actor, float OldDilation, float NewDilation)
{
	UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	const FBodyState* Found = Bodies.Find(Actor);
	const FBodyState Prev = Found ? *Found : FBodyState();
	const bool bSimulating = Root && Root->IsSimulatingPhysics();
	if (!Root || (!Prev.bFrozen && !Prev.bSlowed && !bSimulating))
	{
		// Sin física: con CustomTimeDilation basta.
		Bodies.Remove(Actor);
		return;
	}

	// Velocidad del cuerpo como si fuera a tiempo normal.
	FVector Linear = Prev.LinearVelocity;
	FVector Angular = Prev.AngularVelocity;
	if (!Prev.bFrozen && bSimulating)
	{
		const float ToReal = Prev.bSlowed ? 1.f / FMath::Max(OldDilation, 0.0001f) : 1.f;
		Linear = Root->GetPhysicsLinearVelocity() * ToReal;
		Angular = Root->GetPhysicsAngularVelocityInDegrees() * ToReal;
	}

	FBodyState Next;
	Next.bHadGravity = (Prev.bFrozen || Prev.bSlowed) ? Prev.bHadGravity : Root->IsGravityEnabled();

	if (NewDilation < FrozenBelow)
	{
		Next.bFrozen = true;
		Next.LinearVelocity = Linear;
		Next.AngularVelocity = Angular;
		Next.bDisabledPhysics = Prev.bFrozen ? Prev.bDisabledPhysics : bSimulating;
		Root->SetEnableGravity(Next.bHadGravity);
		if (bSimulating)
		{
			Root->SetSimulatePhysics(false);
		}
		Bodies.Add(Actor, Next);
		return;
	}

	if (Prev.bFrozen && Prev.bDisabledPhysics)
	{
		Root->SetSimulatePhysics(true);
	}

	// En cámara lenta todo pasa NewDilation veces más lento: la velocidad se escala por NewDilation
	// y la gravedad (en Tick) por NewDilation². Aproximación: los choques siguen siendo a velocidad real.
	if (!FMath::IsNearlyEqual(NewDilation, 1.f))
	{
		Next.bSlowed = true;
		Root->SetEnableGravity(false);
		Bodies.Add(Actor, Next);
	}
	else
	{
		Root->SetEnableGravity(Next.bHadGravity);
		Bodies.Remove(Actor);
	}

	// Si el Registro Temporal lo está rebobinando, la física está apagada: no tocamos la velocidad.
	if (Root->IsSimulatingPhysics())
	{
		Root->SetPhysicsLinearVelocity(Linear * NewDilation);
		Root->SetPhysicsAngularVelocityInDegrees(Angular * NewDilation);
	}
}

void UTemporalDilationSubsystem::Tick(float DeltaTime)
{
	// Actores destruidos con pedidos vivos: se olvidan.
	for (auto It = KeyActors.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			Actors.RemoveKey(It.Key());
			It.RemoveCurrent();
		}
	}

	const UWorld* World = GetWorld();
	const float GravityZ = World ? World->GetGravityZ() : -980.f;
	for (auto It = Bodies.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}
		UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
		if (It.Value().bSlowed && It.Value().bHadGravity && Root && Root->IsSimulatingPhysics())
		{
			const float Dilation = Actors.Get(Actor);
			Root->AddForce(FVector(0.f, 0.f, GravityZ * Dilation * Dilation), NAME_None, /*bAccelChange=*/true);
		}
	}

	// El jugador se compensa siempre que el mundo vaya lento o alguien le pida algo.
	if (GetGlobalDilation() < 1.f || (GetPlayer() && Actors.Has(GetPlayer())))
	{
		ApplyPlayer();
	}
}
