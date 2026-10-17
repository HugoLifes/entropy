// ENTROPÍA · Estela ligera

#include "TemporalTrailComponent.h"
#include "TemporalFX.h"

#include "Components/PoseableMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

UTemporalTrailComponent::UTemporalTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Solo tickea mientras hay copias visibles.
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTemporalTrailComponent::BuildPool(USkinnedAsset* Asset)
{
	AActor* Owner = GetOwner();
	for (int32 i = 0; i < PoolSize; ++i)
	{
		UPoseableMeshComponent* Mesh = NewObject<UPoseableMeshComponent>(Owner);
		Mesh->SetSkinnedAssetAndUpdate(Asset);
		Mesh->SetupAttachment(Owner->GetRootComponent());
		// Absolutas: la copia se queda donde nació aunque el jugador se mueva.
		Mesh->SetUsingAbsoluteLocation(true);
		Mesh->SetUsingAbsoluteRotation(true);
		Mesh->SetUsingAbsoluteScale(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetVisibility(false);
		Mesh->RegisterComponent();

		UMaterialInstanceDynamic* Material = TemporalFX::MakeTranslucent(Owner, FLinearColor(1.f, 1.f, 1.f, 0.f));
		TemporalFX::ApplyToAllSlots(Mesh, Material);

		FSlot Slot;
		Slot.Mesh = Mesh;
		Slot.Material = Material;
		Slots.Add(Slot);
		Keep.Add(Mesh);
		Keep.Add(Material);
	}
}

bool UTemporalTrailComponent::ShouldEmit(bool bHasPrevious, const FVector& PreviousLocation, const FVector& NewLocation, float MinMove)
{
	return !bHasPrevious || FVector::DistSquared(PreviousLocation, NewLocation) >= FMath::Square(MinMove);
}

void UTemporalTrailComponent::Emit(USkinnedAsset* Asset, const TArray<FTransform>& Pose, const FTransform& MeshWorldTransform, const FLinearColor& Color)
{
	if (!Asset || !GetOwner())
	{
		return;
	}
	// Quieto: no apilamos copias encima del cuerpo (se veía "transparente" en la Sobremarcha).
	const FVector Location = MeshWorldTransform.GetLocation();
	if (!ShouldEmit(bHasEmitted, LastEmitLocation, Location, MinMoveBetweenCopies))
	{
		return;
	}
	LastEmitLocation = Location;
	bHasEmitted = true;
	if (Slots.Num() == 0)
	{
		BuildPool(Asset);
	}

	// El slot libre o, si no hay, el que lleva más tiempo en pantalla.
	int32 Best = 0;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!Slots[i].bActive)
		{
			Best = i;
			break;
		}
		if (Slots[i].Age > Slots[Best].Age)
		{
			Best = i;
		}
	}

	FSlot& Slot = Slots[Best];
	UPoseableMeshComponent* Mesh = Slot.Mesh.Get();
	if (!Mesh)
	{
		return;
	}
	Mesh->SetWorldTransform(MeshWorldTransform);
	if (Pose.Num() > 0 && Pose.Num() == Mesh->BoneSpaceTransforms.Num())
	{
		Mesh->BoneSpaceTransforms = Pose;
		Mesh->MarkRefreshTransformDirty();
	}
	Mesh->SetVisibility(true);
	if (UMaterialInstanceDynamic* Material = Slot.Material.Get())
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	Slot.Color = Color;
	Slot.Age = 0.f;
	Slot.bActive = true;
	SetComponentTickEnabled(true);
}

void UTemporalTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bAnyActive = false;
	for (FSlot& Slot : Slots)
	{
		if (!Slot.bActive)
		{
			continue;
		}
		Slot.Age += DeltaTime;
		if (Slot.Age >= Life)
		{
			Slot.bActive = false;
			if (UPoseableMeshComponent* Mesh = Slot.Mesh.Get())
			{
				Mesh->SetVisibility(false);
			}
			continue;
		}
		bAnyActive = true;
		// Caída suave (cuadrática): se ve fuerte al nacer y se apaga rápido al final.
		const float Remaining = 1.f - Slot.Age / Life;
		FLinearColor Faded = Slot.Color;
		Faded.A = Slot.Color.A * Remaining * Remaining;
		if (UMaterialInstanceDynamic* Material = Slot.Material.Get())
		{
			Material->SetVectorParameterValue(TEXT("Color"), Faded);
		}
	}

	if (!bAnyActive)
	{
		SetComponentTickEnabled(false);
	}
}
