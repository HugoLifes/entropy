// ENTROPÍA · Efectos visuales compartidos

#include "TemporalFX.h"

#include "Components/MeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	UMaterialInterface* TranslucentBase()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
		}
		return Cached.Get();
	}

	UMaterialInterface* SolidBase()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}

	UNiagaraSystem* BurstSystem()
	{
		static TWeakObjectPtr<UNiagaraSystem> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst"));
		}
		return Cached.Get();
	}
}

UMaterialInstanceDynamic* TemporalFX::MakeTranslucent(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInterface* Base = TranslucentBase();
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	return Material;
}

UMaterialInstanceDynamic* TemporalFX::MakeSolid(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInterface* Base = SolidBase();
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	return Material;
}

void TemporalFX::ApplyToAllSlots(UMeshComponent* Mesh, UMaterialInterface* Material)
{
	if (!Mesh || !Material)
	{
		return;
	}
	for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
	{
		Mesh->SetMaterial(i, Material);
	}
}

void TemporalFX::Burst(const UObject* WorldContext, const FVector& Location, const FLinearColor& Color, float Scale)
{
	UNiagaraSystem* System = BurstSystem();
	if (!System || !WorldContext)
	{
		return;
	}
	if (UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, System, Location, FRotator::ZeroRotator, FVector(Scale)))
	{
		// La plantilla puede o no exponer "Color"; si no existe, se ignora.
		Component->SetVariableLinearColor(TEXT("Color"), Color);
	}
}

// ---------------------------------------------------------------------------

ATemporalAfterimage::ATemporalAfterimage()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	SetRootComponent(Mesh);
}

void ATemporalAfterimage::Init(USkinnedAsset* Asset, const FTransform& MeshWorldTransform, const TArray<FTransform>& Pose, const FLinearColor& Color, float LifeSeconds)
{
	BaseColor = Color;
	Life = FMath::Max(0.05f, LifeSeconds);
	Mesh->SetSkinnedAssetAndUpdate(Asset);
	SetActorTransform(MeshWorldTransform);

	if (Pose.Num() > 0 && Pose.Num() == Mesh->BoneSpaceTransforms.Num())
	{
		Mesh->BoneSpaceTransforms = Pose;
		Mesh->MarkRefreshTransformDirty();
	}

	Material = TemporalFX::MakeTranslucent(this, Color);
	TemporalFX::ApplyToAllSlots(Mesh, Material);
}

void ATemporalAfterimage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;
	if (Material)
	{
		FLinearColor Faded = BaseColor;
		Faded.A = BaseColor.A * FMath::Clamp(1.f - Age / Life, 0.f, 1.f);
		Material->SetVectorParameterValue(TEXT("Color"), Faded);
	}
	if (Age >= Life)
	{
		Destroy();
	}
}
