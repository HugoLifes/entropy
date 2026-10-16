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
#include "Misc/DelayedAutoRegister.h"
#include "Misc/PackageName.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

namespace
{
	// Material propio del proyecto para fantasmas (Eco, estela, burbuja). A diferencia del de depuración
	// del motor, este sí está marcado "Used with Skeletal Mesh": sin eso, en un juego empaquetado el Eco
	// y la estela salían con el material gris por defecto (y en el editor llenaban el log de warnings).
	const TCHAR* GhostPackage = TEXT("/Game/Entropia/Materiales/M_TemporalGhost");
	const TCHAR* GhostObject = TEXT("/Game/Entropia/Materiales/M_TemporalGhost.M_TemporalGhost");

#if WITH_EDITOR
	/** Crea y guarda M_TemporalGhost si todavía no existe: unlit, translúcido, parámetro "Color" (RGB = color, A = opacidad). */
	void CreateGhostMaterialIfMissing()
	{
		if (!GIsEditor || IsRunningCommandlet() || FPackageName::DoesPackageExist(GhostPackage))
		{
			return;
		}

		UPackage* Package = CreatePackage(GhostPackage);
		UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_TemporalGhost"), RF_Public | RF_Standalone);
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);

		UMaterialExpressionVectorParameter* ColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParam->Material = Material;
		ColorParam->ParameterName = TEXT("Color");
		ColorParam->DefaultValue = FLinearColor(0.35f, 0.65f, 1.f, 0.45f);
		Material->GetExpressionCollection().AddExpression(ColorParam);

		// Salidas del VectorParameter: 0 = RGB, 4 = A.
		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, ColorParam);
		Material->GetEditorOnlyData()->Opacity.Connect(4, ColorParam);

		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		FAssetRegistryModule::AssetCreated(Material);
		Package->MarkPackageDirty();

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(GhostPackage, FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(Package, Material, *Filename, Args))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: creado %s"), GhostPackage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ENTROPIA: no se pudo guardar %s (se usará el material de depuración del motor)"), GhostPackage);
		}
	}

	// Corre una vez al terminar de arrancar el editor.
	FDelayedAutoRegisterHelper GCreateGhostMaterial(EDelayedRegisterRunPhase::EndOfEngineInit, [] { CreateGhostMaterialIfMissing(); });
#endif

	UMaterialInterface* TranslucentBase()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, GhostObject, nullptr, LOAD_NoWarn);
		}
		if (!Cached.IsValid())
		{
			// Respaldo: el material de depuración del motor (funciona en el editor, no en un juego empaquetado).
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
