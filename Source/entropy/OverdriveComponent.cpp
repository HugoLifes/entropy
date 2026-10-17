// ENTROPÍA · Sobremarcha

#include "OverdriveComponent.h"
#include "TemporalDilation.h"
#include "TemporalFXSubsystem.h"
#include "InverseStepComponent.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalTrailComponent.h"

#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

UOverdriveComponent::UOverdriveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UOverdriveComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(Owner, TEXT("TemporalEnergy"));
	Trail = TemporalFX::FindOrAddComponent<UTemporalTrailComponent>(Owner, TEXT("TemporalTrail"));

	// Estilo A pero en cian verdoso: así se distingue del azul del Paso Inverso.
	PostProcess = NewObject<UPostProcessComponent>(Owner, TEXT("OverdrivePostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 2.f;
	PostProcess->BlendWeight = 0.f;
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.5f, 0.5f, 0.5f, 1.f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(0.8f, 1.1f, 1.05f, 1.f);
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = 2.5f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.6f;
	PostProcess->RegisterComponent();
}

void UOverdriveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Nunca dejar el mundo en cámara lenta al salir de Play.
	StopOverdrive();
	Super::EndPlay(EndPlayReason);
}

void UOverdriveComponent::Toggle()
{
	if (bOverdrive)
	{
		StopOverdrive();
	}
	else
	{
		StartOverdrive();
	}
}

bool UOverdriveComponent::StartOverdrive()
{
	if (bOverdrive)
	{
		return true;
	}
	if (!Energy || !Energy->CanAfford(MinToStart))
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}

	if (UInverseStepComponent* Inverse = GetOwner()->FindComponentByClass<UInverseStepComponent>())
	{
		Inverse->EndInverseStep();
	}

	bOverdrive = true;
	TrailTimer = 0.f;
	// El árbitro pone el mundo lento y te compensa a ti (si otro poder lo pone más lento, gana ese).
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->RequestGlobal(this, WorldDilation);
	}
	Energy->SetActivePower(TEXT("Sobremarcha"));
	TemporalFX::Burst(this, GetOwner()->GetActorLocation(), FLinearColor(TrailColor.R, TrailColor.G, TrailColor.B), 0.8f);
	// Onda grande hacia afuera: el mundo "se estira" al entrar en Sobremarcha.
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(GetOwner()->GetActorLocation(), 0.04f, 0.8f, 0.8f);
	}
	return true;
}

void UOverdriveComponent::StopOverdrive()
{
	if (!bOverdrive)
	{
		return;
	}
	bOverdrive = false;
	if (UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this))
	{
		FX->Pulse(GetOwner()->GetActorLocation(), -0.02f, 0.4f, 0.3f);
	}
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseGlobal(this);
	}
	if (Energy && Energy->GetActivePower() == TEXT("Sobremarcha"))
	{
		Energy->SetActivePower(NAME_None);
	}
}

void UOverdriveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Todo en tiempo real: la Sobremarcha no debe frenarse a sí misma.
	const float RealDelta = TemporalFX::RealDelta();

	if (bOverdrive)
	{
		if (!Energy || !Energy->Drain(CostPerSecond, RealDelta))
		{
			StopOverdrive();
		}
		else
		{
			TrailTimer += RealDelta;
			if (TrailInterval > 0.f && TrailTimer >= TrailInterval)
			{
				TrailTimer = 0.f;
				EmitTrail();
			}
		}
	}

	EffectAlpha = FMath::FInterpTo(EffectAlpha, bOverdrive ? 1.f : 0.f, RealDelta, bOverdrive ? 12.f : 6.f);
	if (PostProcess)
	{
		PostProcess->BlendWeight = EffectAlpha;
	}
}

void UOverdriveComponent::EmitTrail()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Trail || !Body || !Body->GetSkeletalMeshAsset())
	{
		return;
	}
	Trail->Emit(Body->GetSkeletalMeshAsset(), Body->GetBoneSpaceTransforms(), Body->GetComponentTransform(), TrailColor);
}
