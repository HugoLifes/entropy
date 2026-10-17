// ENTROPÍA · Distorsión del tiempo

#include "TemporalFXSubsystem.h"
#include "TemporalFX.h"

#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

static TAutoConsoleVariable<int32> CVarEntropiaDistortion(
	TEXT("entropia.fx.distortion"), 1,
	TEXT("Distorsión del tiempo (ondas y aura). 1 = encendida, 0 = apagada (máquinas lentas)."));

// ---------------------------------------------------------------------------
// Lógica pura

void FTemporalPulseSet::Add(const FTemporalPulse& Pulse)
{
	if (Pulses.Num() >= MaxPulses)
	{
		int32 Oldest = 0;
		for (int32 i = 1; i < Pulses.Num(); ++i)
		{
			if (Pulses[i].Age > Pulses[Oldest].Age)
			{
				Oldest = i;
			}
		}
		Pulses.RemoveAt(Oldest);
	}
	FTemporalPulse Fresh = Pulse;
	Fresh.Age = 0.f;
	Pulses.Add(Fresh);
}

void FTemporalPulseSet::Advance(float RealDelta)
{
	for (FTemporalPulse& Pulse : Pulses)
	{
		Pulse.Age += RealDelta;
	}
	Pulses.RemoveAll([](const FTemporalPulse& Pulse) { return Pulse.Age >= Pulse.Duration; });
}

float FTemporalPulseSet::RadiusAt(const FTemporalPulse& Pulse)
{
	const float A = Pulse.Duration > 0.f ? FMath::Clamp(Pulse.Age / Pulse.Duration, 0.f, 1.f) : 1.f;
	// Ease-out cúbico: se abre rápido y se frena al final.
	return Pulse.MaxRadius * (1.f - FMath::Pow(1.f - A, 3.f));
}

float FTemporalPulseSet::StrengthAt(const FTemporalPulse& Pulse)
{
	const float A = Pulse.Duration > 0.f ? FMath::Clamp(Pulse.Age / Pulse.Duration, 0.f, 1.f) : 1.f;
	return Pulse.Strength * (1.f - A);
}

// ---------------------------------------------------------------------------
// Subsystem

UTemporalFXSubsystem* UTemporalFXSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTemporalFXSubsystem>() : nullptr;
}

bool UTemporalFXSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTemporalFXSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTemporalFXSubsystem, STATGROUP_Tickables);
}

void UTemporalFXSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMaterialInterface* Base = TemporalFX::DistortionMaterial();
	if (!Base)
	{
		return;
	}
	Material = UMaterialInstanceDynamic::Create(Base, this);

	// Un actor mínimo que solo lleva el post-process (sin límites: afecta a toda la pantalla).
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	HostActor = InWorld.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!HostActor)
	{
		return;
	}
	PostProcess = NewObject<UPostProcessComponent>(HostActor, TEXT("TemporalDistortion"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 3.f;
	PostProcess->BlendWeight = 0.f;
	PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Material));
	HostActor->SetRootComponent(PostProcess);
	PostProcess->RegisterComponent();
}

void UTemporalFXSubsystem::Pulse(const FVector& WorldLocation, float Strength, float Duration, float MaxRadius)
{
	FTemporalPulse P;
	P.World = WorldLocation;
	P.Strength = Strength;
	P.Duration = FMath::Max(0.05f, Duration);
	P.MaxRadius = MaxRadius;
	Pulses.Add(P);
}

void UTemporalFXSubsystem::SetAura(AActor* Target, float Strength)
{
	if (Target && Strength > 0.f)
	{
		AuraTarget = Target;
		AuraStrength = Strength;
		AuraFadeStrength = Strength;
	}
	else
	{
		// No borramos el objetivo de golpe: el aura se apaga suave siguiendo al mismo actor.
		AuraStrength = 0.f;
	}
}

void UTemporalFXSubsystem::Tick(float DeltaTime)
{
	const float RealDelta = TemporalFX::RealDelta();
	Pulses.Advance(RealDelta);
	AuraBlend = FMath::FInterpTo(AuraBlend, AuraStrength > 0.f ? 1.f : 0.f, RealDelta, AuraStrength > 0.f ? 10.f : 5.f);

	if (!PostProcess || !Material)
	{
		return;
	}

	bool bAnyActive = false;
	if (CVarEntropiaDistortion.GetValueOnGameThread() != 0)
	{
		ApplyToMaterial(bAnyActive);
	}
	// Sin efectos: peso 0 y el material ni se dibuja.
	PostProcess->BlendWeight = bAnyActive ? 1.f : 0.f;
}

void UTemporalFXSubsystem::ApplyToMaterial(bool& bOutAnyActive)
{
	bOutAnyActive = false;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}
	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		return;
	}

	auto ToScreen = [PC, SizeX, SizeY](const FVector& World, FVector2D& OutUV)
	{
		FVector2D Screen;
		if (!UGameplayStatics::ProjectWorldToScreen(PC, World, Screen, /*bPlayerViewportRelative=*/true))
		{
			return false;
		}
		OutUV = FVector2D(Screen.X / SizeX, Screen.Y / SizeY);
		return true;
	};

	static const FName PulseNames[FTemporalPulseSet::MaxPulses] = { TEXT("Pulse0"), TEXT("Pulse1"), TEXT("Pulse2"), TEXT("Pulse3") };
	for (int32 i = 0; i < FTemporalPulseSet::MaxPulses; ++i)
	{
		FLinearColor Value(0.f, 0.f, 0.f, 0.f);
		FVector2D UV;
		if (Pulses.Pulses.IsValidIndex(i) && ToScreen(Pulses.Pulses[i].World, UV))
		{
			const FTemporalPulse& P = Pulses.Pulses[i];
			Value = FLinearColor(UV.X, UV.Y, FTemporalPulseSet::RadiusAt(P), FTemporalPulseSet::StrengthAt(P));
			bOutAnyActive = true;
		}
		Material->SetVectorParameterValue(PulseNames[i], Value);
	}

	FLinearColor Aura(0.f, 0.f, 0.f, 0.f);
	FVector2D AuraUV;
	if (AuraBlend > 0.001f && AuraTarget.IsValid() && ToScreen(AuraTarget->GetActorLocation(), AuraUV))
	{
		Aura = FLinearColor(AuraUV.X, AuraUV.Y, 0.2f, AuraFadeStrength * AuraBlend);
		bOutAnyActive = true;
	}
	Material->SetVectorParameterValue(TEXT("Aura"), Aura);
}
