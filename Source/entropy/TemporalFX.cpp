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
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
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

	// Post-process de distorsión del tiempo (ondas + aura). Lo maneja UTemporalFXSubsystem.
	const TCHAR* DistortionPackage = TEXT("/Game/Entropia/Materiales/M_TemporalDistortion");
	const TCHAR* DistortionObject = TEXT("/Game/Entropia/Materiales/M_TemporalDistortion.M_TemporalDistortion");

	// Polvo de los escombros: translúcido con bordes suaves y permiso para InstancedStaticMesh
	// (sin ese permiso el motor dibujaba el polvo con el material gris por defecto: bolas sólidas).
	const TCHAR* DustPackage = TEXT("/Game/Entropia/Materiales/M_TemporalDust");
	const TCHAR* DustObject = TEXT("/Game/Entropia/Materiales/M_TemporalDust.M_TemporalDust");

#if WITH_EDITOR
	/** Compila, registra y guarda en disco un material recién creado. */
	void SaveNewMaterial(UPackage* Package, UMaterial* Material, const TCHAR* PackageName)
	{
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		FAssetRegistryModule::AssetCreated(Material);
		Package->MarkPackageDirty();

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(Package, Material, *Filename, Args))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: creado %s"), PackageName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ENTROPIA: no se pudo guardar %s"), PackageName);
		}
	}

	/** Crea y guarda M_TemporalGhost si todavía no existe: unlit, translúcido, parámetro "Color" (RGB = color, A = opacidad). */
	void CreateGhostMaterialIfMissing()
	{
		if (FPackageName::DoesPackageExist(GhostPackage))
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
		SaveNewMaterial(Package, Material, GhostPackage);
	}

	/**
	 * Crea M_TemporalDistortion: post-process con un nodo Custom (HLSL) que dobla la imagen con hasta 4 ondas
	 * (Pulse0..3: x, y = centro en UV de pantalla; z = radio; w = fuerza) y un aura ondulante (Aura), con aberración de color.
	 */
	void CreateDistortionMaterialIfMissing()
	{
		if (FPackageName::DoesPackageExist(DistortionPackage))
		{
			return;
		}

		UPackage* Package = CreatePackage(DistortionPackage);
		UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_TemporalDistortion"), RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;

		UMaterialExpressionCustom* Custom = NewObject<UMaterialExpressionCustom>(Material);
		Custom->Material = Material;
		Custom->Description = TEXT("TemporalDistortion");
		Custom->OutputType = CMOT_Float3;
		Custom->Code = TEXT(R"HLSL(
float2 BufUV = GetDefaultSceneTextureUV(Parameters, 14);
float2 ViewUV = (BufUV * View.BufferSizeAndInvSize.xy - View.ViewRectMin.xy) * View.ViewSizeAndInvSize.zw;
float Aspect = View.ViewSizeAndInvSize.x / max(View.ViewSizeAndInvSize.y, 1.0);
float2 Off = float2(0.0, 0.0);
float Chroma = 0.0;
float4 P[4] = { Pulse0, Pulse1, Pulse2, Pulse3 };
for (int i = 0; i < 4; ++i)
{
	float2 D = ViewUV - P[i].xy;
	D.x *= Aspect;
	float Dist = length(D);
	float Band = saturate(1.0 - abs(Dist - P[i].z) / max(Thickness, 0.001));
	Band = Band * Band * (3.0 - 2.0 * Band);
	float2 Dir = Dist > 0.0001 ? D / Dist : float2(0.0, 0.0);
	Dir.x /= Aspect;
	Off += Dir * Band * P[i].w;
	Chroma += Band * abs(P[i].w);
}
{
	float2 D = ViewUV - Aura.xy;
	D.x *= Aspect;
	float Dist = length(D);
	float Mask = saturate(1.0 - Dist / max(Aura.z, 0.001));
	Mask *= Mask;
	float Wave = sin(Dist * 90.0 - View.RealTime * 7.0) * 0.6 + sin(atan2(D.y, D.x) * 6.0 + View.RealTime * 3.0) * 0.4;
	float2 Dir = Dist > 0.0001 ? D / Dist : float2(0.0, 0.0);
	Dir.x /= Aspect;
	Off += Dir * Wave * Mask * Aura.w;
	Chroma += Mask * abs(Aura.w) * 0.6;
}
float2 OffBuf = Off * View.ViewSizeAndInvSize.xy * View.BufferSizeAndInvSize.zw;
float Split = ChromaBase + Chroma * ChromaGain;
float R = SceneTextureLookup(BufUV + OffBuf * (1.0 + Split), 14, false).r;
float G = SceneTextureLookup(BufUV + OffBuf, 14, false).g;
float B = SceneTextureLookup(BufUV + OffBuf * (1.0 - Split), 14, false).b;
return float3(R, G, B) + Dummy.rgb * 0.0;
)HLSL");

		auto AddVector = [Material](const TCHAR* Name)
		{
			UMaterialExpressionVectorParameter* Param = NewObject<UMaterialExpressionVectorParameter>(Material);
			Param->Material = Material;
			Param->ParameterName = Name;
			Param->DefaultValue = FLinearColor(0.f, 0.f, 0.f, 0.f);
			Material->GetExpressionCollection().AddExpression(Param);
			return Param;
		};

		Custom->Inputs.Reset();
		const TCHAR* VectorInputs[] = { TEXT("Pulse0"), TEXT("Pulse1"), TEXT("Pulse2"), TEXT("Pulse3"), TEXT("Aura") };
		for (const TCHAR* Name : VectorInputs)
		{
			FCustomInput Input;
			Input.InputName = Name;
			// Salida 5 del VectorParameter = RGBA (float4).
			Input.Input.Connect(5, AddVector(Name));
			Custom->Inputs.Add(Input);
		}

		UMaterialExpressionScalarParameter* Thickness = NewObject<UMaterialExpressionScalarParameter>(Material);
		Thickness->Material = Material;
		Thickness->ParameterName = TEXT("Thickness");
		Thickness->DefaultValue = 0.06f;
		Material->GetExpressionCollection().AddExpression(Thickness);
		FCustomInput ThicknessInput;
		ThicknessInput.InputName = TEXT("Thickness");
		ThicknessInput.Input.Connect(0, Thickness);
		Custom->Inputs.Add(ThicknessInput);

		// Aberración de color ajustable sin regenerar el material (la primera versión era demasiado "arcoíris").
		const TPair<const TCHAR*, float> ChromaParams[] = { { TEXT("ChromaBase"), 0.1f }, { TEXT("ChromaGain"), 6.f } };
		for (const TPair<const TCHAR*, float>& Pair : ChromaParams)
		{
			UMaterialExpressionScalarParameter* Param = NewObject<UMaterialExpressionScalarParameter>(Material);
			Param->Material = Material;
			Param->ParameterName = Pair.Key;
			Param->DefaultValue = Pair.Value;
			Material->GetExpressionCollection().AddExpression(Param);
			FCustomInput Input;
			Input.InputName = Pair.Key;
			Input.Input.Connect(0, Param);
			Custom->Inputs.Add(Input);
		}

		// Un SceneTexture(PostProcessInput0) conectado para que la imagen quede ligada al material (SceneTextureLookup la lee).
		UMaterialExpressionSceneTexture* Scene = NewObject<UMaterialExpressionSceneTexture>(Material);
		Scene->Material = Material;
		Scene->SceneTextureId = PPI_PostProcessInput0;
		Material->GetExpressionCollection().AddExpression(Scene);
		FCustomInput DummyInput;
		DummyInput.InputName = TEXT("Dummy");
		DummyInput.Input.Connect(0, Scene);
		Custom->Inputs.Add(DummyInput);

		Material->GetExpressionCollection().AddExpression(Custom);
		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Custom);
		SaveNewMaterial(Package, Material, DistortionPackage);
	}

	/** Crea M_TemporalDust: unlit, translúcido, opacidad = Color.A × (1 − Fresnel) para bordes suaves; apto para ISM. */
	void CreateDustMaterialIfMissing()
	{
		if (FPackageName::DoesPackageExist(DustPackage))
		{
			return;
		}

		UPackage* Package = CreatePackage(DustPackage);
		UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_TemporalDust"), RF_Public | RF_Standalone);
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);

		UMaterialExpressionVectorParameter* ColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParam->Material = Material;
		ColorParam->ParameterName = TEXT("Color");
		ColorParam->DefaultValue = FLinearColor(0.75f, 0.72f, 0.66f, 0.3f);
		Material->GetExpressionCollection().AddExpression(ColorParam);

		UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(Material);
		Fresnel->Material = Material;
		Fresnel->Exponent = 2.f;
		Material->GetExpressionCollection().AddExpression(Fresnel);

		UMaterialExpressionOneMinus* Soft = NewObject<UMaterialExpressionOneMinus>(Material);
		Soft->Material = Material;
		Soft->Input.Connect(0, Fresnel);
		Material->GetExpressionCollection().AddExpression(Soft);

		UMaterialExpressionMultiply* Alpha = NewObject<UMaterialExpressionMultiply>(Material);
		Alpha->Material = Material;
		Alpha->A.Connect(4, ColorParam);
		Alpha->B.Connect(0, Soft);
		Material->GetExpressionCollection().AddExpression(Alpha);

		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, ColorParam);
		Material->GetEditorOnlyData()->Opacity.Connect(0, Alpha);
		SaveNewMaterial(Package, Material, DustPackage);
	}

	// Corre una vez al terminar de arrancar el motor con el editor (también con -game desde el editor, para el tour).
	FDelayedAutoRegisterHelper GCreateEntropiaMaterials(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (IsRunningCommandlet())
		{
			return;
		}
		CreateGhostMaterialIfMissing();
		CreateDistortionMaterialIfMissing();
		CreateDustMaterialIfMissing();
	});
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

UMaterialInterface* TemporalFX::DistortionMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, DistortionObject, nullptr, LOAD_NoWarn);
	}
	return Cached.Get();
}

UMaterialInstanceDynamic* TemporalFX::MakeDust(UObject* Outer, const FLinearColor& Color)
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, DustObject, nullptr, LOAD_NoWarn);
	}
	if (!Cached.IsValid())
	{
		return MakeTranslucent(Outer, Color);
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Cached.Get(), Outer);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	return Material;
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
