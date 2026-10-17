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

	// 1) Lo que está justo bajo la mira gana siempre (cajas, plataformas, pedazos: todo lo que bloquea la vista).
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ObjectInversionAimDirect), false, Pawn);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + Forward * MaxDistance, ECC_Visibility, Params))
		{
			if (UTemporalRecorderComponent* Direct = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UTemporalRecorderComponent>() : nullptr)
			{
				return Direct;
			}
		}
	}

	// 2) Si no, el más cercano a la mira por ángulo (así entran las balas, que no bloquean la vista).
	//    Los que cubren la mira empatan en 0 y desempata el centro más cercano: una plataforma enorme junto
	//    a ti ya no le gana a la bala que estás apuntando.
	struct FCandidate
	{
		UTemporalRecorderComponent* Recorder;
		FVector Center;
		float EdgeAngle;
		float CenterAngle;
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
			Candidates.Add({ Recorder, Center, EdgeAngle, CenterAngle });
		}
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return IsBetterAim(A.EdgeAngle, A.CenterAngle, B.EdgeAngle, B.CenterAngle); });

	// El primero que se vea de verdad. Si lo que tapa la vista es de su mismo grupo (otro pedazo del mismo
	// objeto roto), cuenta como visible: así un montón de escombros sí se puede apuntar.
	for (int32 i = 0; i < FMath::Min(Candidates.Num(), 4); ++i)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ObjectInversionAim), false, Pawn);
		Params.AddIgnoredActor(Candidates[i].Recorder->GetOwner());
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, Candidates[i].Center, ECC_Visibility, Params))
		{
			return Candidates[i].Recorder;
		}
		const UTemporalRecorderComponent* Blocker = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UTemporalRecorderComponent>() : nullptr;
		if (Blocker)
		{
			TSet<UTemporalRecorderComponent*> Group;
			Candidates[i].Recorder->GatherGroup(Group);
			if (Group.Contains(Blocker))
			{
				return Candidates[i].Recorder;
			}
		}
	}
	return nullptr;
}

bool UObjectInversionComponent::IsBetterAim(float EdgeAngleA, float CenterAngleA, float EdgeAngleB, float CenterAngleB)
{
	// Un borde "negativo" solo significa que el objeto cubre la mira: no debe premiar a los grandes o cercanos.
	const float ScoreA = FMath::Max(EdgeAngleA, 0.f);
	const float ScoreB = FMath::Max(EdgeAngleB, 0.f);
	if (!FMath::IsNearlyEqual(ScoreA, ScoreB, 0.01f))
	{
		return ScoreA < ScoreB;
	}
	return CenterAngleA < CenterAngleB;
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
		const bool bHasEnergy = Energy && Energy->Drain(CostPerSecond, TemporalFX::RealDelta());
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
