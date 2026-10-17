// ENTROPÍA · Detener el tiempo, nivel 2

#include "TemporalWorldStopComponent.h"
#include "TemporalDilation.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"

#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace EntropiaWorldStop
{
	float WaveRadius(float T, float Duration, float MaxRadius)
	{
		if (T <= 0.f || T >= Duration)
		{
			return 0.f;
		}
		auto Ease = [](float A)
		{
			const float X = FMath::Clamp(A, 0.f, 1.f);
			return 1.f - (1.f - X) * (1.f - X);
		};
		if (T < ExpandSeconds)
		{
			return MaxRadius * Ease(T / ExpandSeconds);
		}
		const float Left = Duration - T;
		if (Left < ReturnSeconds)
		{
			return MaxRadius * Ease(Left / ReturnSeconds);
		}
		return MaxRadius;
	}

	float Flash(float T)
	{
		return T < 0.f ? 0.f : FMath::Clamp(1.f - T / FlashSeconds, 0.f, 1.f);
	}
}

UTemporalWorldStopComponent::UTemporalWorldStopComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTemporalWorldStopComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(Owner, TEXT("TemporalEnergy"));

	PostProcess = NewObject<UPostProcessComponent>(Owner, TEXT("WorldStopPostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 4.f;
	PostProcess->BlendWeight = 0.f;
	if (UMaterialInterface* Base = TemporalFX::WorldStopMaterial())
	{
		Effect = UMaterialInstanceDynamic::Create(Base, this);
		PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Effect));
	}
	PostProcess->RegisterComponent();
	Aura = TemporalFX::MakeTranslucent(this, FLinearColor(0.7f, 0.2f, 1.f, 0.25f));
}

void UTemporalWorldStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Stop();
	Super::EndPlay(EndPlayReason);
}

void UTemporalWorldStopComponent::SetPlayerHighlight(bool bOn)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body)
	{
		return;
	}
	// Custom Stencil 1: el post-proceso deja a color lo que lo tiene (solo tú).
	Body->SetRenderCustomDepth(bOn);
	Body->SetCustomDepthStencilValue(bOn ? 1 : 0);
	Body->SetOverlayMaterial(bOn ? Aura.Get() : nullptr);
}

bool UTemporalWorldStopComponent::Start()
{
	if (bActive)
	{
		return false;
	}
	if (!Energy || !Energy->Spend(Cost))
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}
	bActive = true;
	Elapsed = 0.f;
	Origin = GetOwner()->GetActorLocation();
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->RequestGlobal(this, WorldDilation);
	}
	SetPlayerHighlight(true);
	Energy->SetActivePower(TEXT("MundoDetenido"));
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(Origin, 0.06f, 0.8f, 1.f);
	}
	UpdateLook();
	return true;
}

void UTemporalWorldStopComponent::Stop()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseGlobal(this);
	}
	SetPlayerHighlight(false);
	if (PostProcess)
	{
		PostProcess->BlendWeight = 0.f;
	}
	if (Energy && Energy->GetActivePower() == TEXT("MundoDetenido"))
	{
		Energy->SetActivePower(NAME_None);
	}
	if (GetOwner())
	{
		if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
		{
			FX->Pulse(GetOwner()->GetActorLocation(), -0.04f, 0.5f, 0.6f);
		}
	}
}

void UTemporalWorldStopComponent::UpdateLook()
{
	if (!Effect || !PostProcess)
	{
		return;
	}
	const float Radius = EntropiaWorldStop::WaveRadius(Elapsed, Duration, MaxRadius);
	// Con la onda casi al máximo, todo queda congelado (también el cielo, que está "infinitamente" lejos).
	const float Fill = FMath::Clamp((Radius / MaxRadius - 0.9f) / 0.1f, 0.f, 1.f);
	Effect->SetVectorParameterValue(TEXT("StopCenter"), FLinearColor(Origin.X, Origin.Y, Origin.Z, 0.f));
	Effect->SetVectorParameterValue(TEXT("StopParams"), FLinearColor(Radius, Fill, EntropiaWorldStop::Flash(Elapsed), 1.f));
	PostProcess->BlendWeight = 1.f;

	// Aura viva: púrpura → dorado → cian.
	if (Aura)
	{
		const FLinearColor Colors[] = { FLinearColor(0.7f, 0.15f, 1.f), FLinearColor(1.f, 0.75f, 0.15f), FLinearColor(0.15f, 0.9f, 1.f) };
		const float Phase = FMath::Fmod(Elapsed * 1.5f, 3.f);
		const int32 From = FMath::FloorToInt(Phase);
		FLinearColor Color = FMath::Lerp(Colors[From % 3], Colors[(From + 1) % 3], Phase - From);
		Color.A = 0.28f;
		Aura->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void UTemporalWorldStopComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bActive)
	{
		return;
	}
	// Tiempo real: el mundo está detenido, tú no.
	Elapsed += TemporalFX::RealDelta();
	if (Elapsed >= Duration)
	{
		Stop();
		return;
	}
	UpdateLook();
}
