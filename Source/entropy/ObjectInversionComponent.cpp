// ENTROPÍA · Invertir objeto

#include "ObjectInversionComponent.h"
#include "InverseStepComponent.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"

UObjectInversionComponent::UObjectInversionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UObjectInversionComponent::BeginPlay()
{
	Super::BeginPlay();
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(GetOwner(), TEXT("TemporalEnergy"));
	HighlightMaterial = TemporalFX::MakeTranslucent(this, HighlightColor);
}

void UObjectInversionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndInversion();
	ClearHighlight();
	Super::EndPlay(EndPlayReason);
}

UTemporalRecorderComponent* UObjectInversionComponent::FindTarget() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	if (!PC || !Subsystem)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector Forward = ViewRotation.Vector();

	// Candidatos por ángulo desde la mira. Se mide desde el borde del objeto: las plataformas grandes
	// son fáciles de apuntar y las balas o los pedazos pequeños también entran con los 4°.
	struct FCandidate
	{
		UTemporalRecorderComponent* Recorder;
		FVector Center;
		float Angle;
	};
	TArray<FCandidate> Candidates;
	for (UTemporalRecorderComponent* Recorder : Subsystem->GetRecorders())
	{
		const AActor* Target = Recorder->GetOwner();
		if (!IsValid(Target) || Target == Pawn)
		{
			continue;
		}
		FVector Center, Extent;
		Target->GetActorBounds(false, Center, Extent);
		const FVector ToTarget = Center - ViewLocation;
		const float Distance = ToTarget.Size();
		if (Distance < 1.f || Distance > MaxDistance)
		{
			continue;
		}
		const float CenterAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToTarget / Distance, Forward), -1.f, 1.f)));
		const float Radius = FMath::Min(Extent.Size(), Distance * 0.9f);
		const float EdgeAngle = CenterAngle - FMath::RadiansToDegrees(FMath::Atan(Radius / Distance));
		if (EdgeAngle < MaxAngleDegrees)
		{
			Candidates.Add({ Recorder, Center, EdgeAngle });
		}
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Angle < B.Angle; });

	// El primero que se vea de verdad (sin pared en medio). Pocos traces: solo los mejores candidatos.
	for (int32 i = 0; i < FMath::Min(Candidates.Num(), 4); ++i)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ObjectInversionAim), false, Pawn);
		Params.AddIgnoredActor(Candidates[i].Recorder->GetOwner());
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, Candidates[i].Center, ECC_Visibility, Params))
		{
			return Candidates[i].Recorder;
		}
	}
	return nullptr;
}

void UObjectInversionComponent::HighlightGroupOf(UTemporalRecorderComponent* Target)
{
	ClearHighlight();
	CurrentTarget = Target;
	if (!Target || !HighlightMaterial)
	{
		return;
	}

	TSet<UTemporalRecorderComponent*> Group;
	Target->GatherGroup(Group);
	for (UTemporalRecorderComponent* Member : Group)
	{
		TArray<UMeshComponent*> Meshes;
		Member->GetOwner()->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh->IsVisible())
			{
				Mesh->SetOverlayMaterial(HighlightMaterial);
				Highlighted.Add(Mesh);
			}
		}
	}
}

void UObjectInversionComponent::ClearHighlight()
{
	for (const TWeakObjectPtr<UMeshComponent>& Mesh : Highlighted)
	{
		if (Mesh.IsValid())
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	Highlighted.Reset();
	CurrentTarget = nullptr;
}

bool UObjectInversionComponent::BeginInversion()
{
	if (bInverting)
	{
		return true;
	}

	UTemporalRecorderComponent* Target = CurrentTarget.IsValid() ? CurrentTarget.Get() : FindTarget();
	if (!Target || !Energy || !Energy->CanAfford(MinToStart))
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}

	// F y R no van juntos: empezar a invertir un objeto suelta el Paso Inverso.
	if (UInverseStepComponent* Inverse = GetOwner()->FindComponentByClass<UInverseStepComponent>())
	{
		Inverse->EndInverseStep();
	}

	HighlightGroupOf(Target);
	TSet<UTemporalRecorderComponent*> Group;
	Target->GatherGroup(Group);
	ActiveGroup.Reset();
	for (UTemporalRecorderComponent* Member : Group)
	{
		Member->StartRewind();
		ActiveGroup.Add(Member);
	}

	bInverting = true;
	Energy->SetActivePower(TEXT("Inversion"));
	TemporalFX::Burst(this, Target->GetOwner()->GetActorLocation(), FLinearColor(1.f, 0.65f, 0.15f), 0.5f);
	return true;
}

void UObjectInversionComponent::EndInversion()
{
	if (!bInverting)
	{
		return;
	}
	bInverting = false;

	for (const TWeakObjectPtr<UTemporalRecorderComponent>& Member : ActiveGroup)
	{
		if (Member.IsValid() && Member->IsRewinding())
		{
			Member->StopRewind();
		}
	}
	ActiveGroup.Reset();

	if (Energy && Energy->GetActivePower() == TEXT("Inversion"))
	{
		Energy->SetActivePower(NAME_None);
	}
}

void UObjectInversionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bInverting)
	{
		// Termina solo si se acaba la energía o si ya nadie del grupo se está rebobinando
		// (llegó al inicio de su grabación, o la bala volvió al cañón y desapareció).
		bool bAnyRewinding = false;
		for (const TWeakObjectPtr<UTemporalRecorderComponent>& Member : ActiveGroup)
		{
			bAnyRewinding |= Member.IsValid() && Member->IsRewinding();
		}
		const bool bHasEnergy = Energy && Energy->Drain(CostPerSecond, FApp::GetDeltaTime());
		if (!bHasEnergy || !bAnyRewinding)
		{
			EndInversion();
		}
		return;
	}

	// Mientras no inviertes, el resaltado sigue a la mira.
	UTemporalRecorderComponent* Target = FindTarget();
	if (Target != CurrentTarget.Get())
	{
		HighlightGroupOf(Target);
	}
}
