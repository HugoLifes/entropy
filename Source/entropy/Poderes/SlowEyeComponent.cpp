// ENTROPÍA · Ojo lento

#include "SlowEyeComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalDilation.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalFXSubsystem.h"
#include "TemporalProjectile.h"

#include "EngineUtils.h"

USlowEyeComponent::USlowEyeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USlowEyeComponent::BeginPlay()
{
	Super::BeginPlay();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(GetOwner(), TEXT("TemporalEnergy"));
	Aim = TemporalFX::FindOrAddComponent<UTemporalAimComponent>(GetOwner(), TEXT("TemporalAim"));
}

void USlowEyeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAll();
	Super::EndPlay(EndPlayReason);
}

void USlowEyeComponent::ReleaseAll()
{
	if (Slowed.Num() == 0)
	{
		return;
	}
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseSource(this);
	}
	Slowed.Reset();
	if (Energy && Energy->GetActivePower() == TEXT("OjoLento"))
	{
		Energy->SetActivePower(NAME_None);
	}
}

void USlowEyeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
	if (!Dilation || !Aim || !Aim->IsAiming() || !Energy || Energy->Energy <= 0.f)
	{
		ReleaseAll();
		return;
	}

	// Las balas que están cerca ahora.
	const FVector Eye = GetOwner()->GetActorLocation();
	TArray<AActor*> InRange;
	for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
	{
		// Fase 4: tus propias balas no (ni las que disparas ni las que dejaste en la pared).
		if (It->bFromPlayer)
		{
			continue;
		}
		if (FVector::DistSquared(It->GetActorLocation(), Eye) <= FMath::Square(Radius))
		{
			InRange.Add(*It);
		}
	}

	// Las que salieron (o desaparecieron) vuelven a su velocidad.
	for (int32 i = Slowed.Num() - 1; i >= 0; --i)
	{
		AActor* Bullet = Slowed[i].Get();
		if (!Bullet || !InRange.Contains(Bullet))
		{
			if (Bullet)
			{
				Dilation->Release(Bullet, this);
			}
			Slowed.RemoveAt(i);
		}
	}

	// Las que entraron se frenan (con una onda chica para que se note).
	UTemporalFXSubsystem* FX = UTemporalFXSubsystem::Get(this);
	for (AActor* Bullet : InRange)
	{
		if (!Slowed.Contains(Bullet))
		{
			Dilation->Request(Bullet, this, BulletDilation);
			Slowed.Add(Bullet);
			if (FX)
			{
				FX->Pulse(Bullet->GetActorLocation(), -0.015f, 0.3f, 0.08f);
			}
		}
	}

	// Gasta solo mientras frena algo.
	if (Slowed.Num() == 0)
	{
		if (Energy->GetActivePower() == TEXT("OjoLento"))
		{
			Energy->SetActivePower(NAME_None);
		}
		return;
	}
	if (!Energy->Drain(CostPerSecond, TemporalFX::RealDelta()))
	{
		ReleaseAll();
		return;
	}
	if (Energy->GetActivePower() == NAME_None)
	{
		Energy->SetActivePower(TEXT("OjoLento"));
	}
}
