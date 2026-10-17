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
#include "Materials/MaterialExpressionWorldPosition.h"
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

	// Esfera de la burbuja: borde brillante (Fresnel) y centro casi transparente, visible desde adentro y desde afuera.
	const TCHAR* FieldPackage = TEXT("/Game/Entropia/Materiales/M_TemporalField");
	const TCHAR* FieldObject = TEXT("/Game/Entropia/Materiales/M_TemporalField.M_TemporalField");

	// Detener nivel 2 (Star Platinum: The World): onda en negativo, mundo gris e índigo, el jugador a color (Custom Stencil).
	const TCHAR* WorldStopPackage = TEXT("/Game/Entropia/Materiales/M_TemporalWorldStop");
	const TCHAR* WorldStopObject = TEXT("/Game/Entropia/Materiales/M_TemporalWorldStop.M_TemporalWorldStop");

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
float2 ViewUV = GetViewportUV(Parameters);
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
float Split = ChromaBase + Chroma * ChromaGain;
float2 UvR = ClampSceneTextureUV(ViewportUVToSceneTextureUV(ViewUV + Off * (1.0 + Split), 14), 14);
float2 UvG = ClampSceneTextureUV(ViewportUVToSceneTextureUV(ViewUV + Off, 14), 14);
float2 UvB = ClampSceneTextureUV(ViewportUVToSceneTextureUV(ViewUV + Off * (1.0 - Split), 14), 14);
float R = SceneTextureLookup(UvR, 14, true).r;
float G = SceneTextureLookup(UvG, 14, true).g;
float B = SceneTextureLookup(UvB, 14, true).b;
float3 Col = float3(R, G, B);
// Campos de tiempo detenido: lo que está DENTRO de la esfera (posición real del píxel) se ve desaturado y frío,
// con un leve brillo en la superficie de la esfera.
float4 Fld[2] = { FieldA, FieldB };
float FStr[2] = { FieldStrength.x, FieldStrength.y };
for (int f = 0; f < 2; ++f)
{
	if (Fld[f].w > 1.0 && FStr[f] > 0.001)
	{
		float Dist = distance(WorldPos, Fld[f].xyz);
		float Inside = 1.0 - smoothstep(Fld[f].w * 0.96, Fld[f].w, Dist);
		float Lum = dot(Col, float3(0.3, 0.59, 0.11));
		float3 Frozen = lerp(float3(Lum, Lum, Lum), FieldTint.rgb * Lum * 1.5, 0.45);
		Col = lerp(Col, Frozen, Inside * FStr[f] * FieldTint.a);
		float Shell = 1.0 - saturate(abs(Dist - Fld[f].w) / (Fld[f].w * 0.035));
		Col += FieldTint.rgb * Shell * 0.18 * FStr[f];
	}
}
return Col + Dummy.rgb * 0.0;
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
		const TCHAR* VectorInputs[] = { TEXT("Pulse0"), TEXT("Pulse1"), TEXT("Pulse2"), TEXT("Pulse3"), TEXT("Aura"),
			TEXT("FieldA"), TEXT("FieldB"), TEXT("FieldStrength") };
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

		// Color del "congelado" (RGB) y cuánto se nota (A).
		UMaterialExpressionVectorParameter* Tint = AddVector(TEXT("FieldTint"));
		Tint->DefaultValue = FLinearColor(0.55f, 0.85f, 1.f, 0.85f);
		FCustomInput TintInput;
		TintInput.InputName = TEXT("FieldTint");
		TintInput.Input.Connect(5, Tint);
		Custom->Inputs.Add(TintInput);

		// Posición del mundo del píxel (sale de la profundidad): para saber qué está dentro de cada burbuja.
		UMaterialExpressionWorldPosition* World = NewObject<UMaterialExpressionWorldPosition>(Material);
		World->Material = Material;
		Material->GetExpressionCollection().AddExpression(World);
		FCustomInput WorldInput;
		WorldInput.InputName = TEXT("WorldPos");
		WorldInput.Input.Connect(0, World);
		Custom->Inputs.Add(WorldInput);

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

	/** Crea M_TemporalField: unlit, translúcido, dos caras; opacidad = Color.A × Fresnel (borde brillante, centro claro). */
	void CreateFieldMaterialIfMissing()
	{
		if (FPackageName::DoesPackageExist(FieldPackage))
		{
			return;
		}

		UPackage* Package = CreatePackage(FieldPackage);
		UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_TemporalField"), RF_Public | RF_Standalone);
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialExpressionVectorParameter* ColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParam->Material = Material;
		ColorParam->ParameterName = TEXT("Color");
		ColorParam->DefaultValue = FLinearColor(0.3f, 0.85f, 1.f, 0.5f);
		Material->GetExpressionCollection().AddExpression(ColorParam);

		UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(Material);
		Fresnel->Material = Material;
		Fresnel->Exponent = 3.f;
		Fresnel->BaseReflectFraction = 0.06f;
		Material->GetExpressionCollection().AddExpression(Fresnel);

		UMaterialExpressionMultiply* Alpha = NewObject<UMaterialExpressionMultiply>(Material);
		Alpha->Material = Material;
		Alpha->A.Connect(4, ColorParam);
		Alpha->B.Connect(0, Fresnel);
		Material->GetExpressionCollection().AddExpression(Alpha);

		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, ColorParam);
		Material->GetEditorOnlyData()->Opacity.Connect(0, Alpha);
		SaveNewMaterial(Package, Material, FieldPackage);
	}

	/**
	 * Crea M_TemporalWorldStop: post-process (después del tonemapping) para Detener nivel 2.
	 *   StopCenter (xyz) = de dónde sale la onda · StopRadius = radio actual · StopFill = 0..1 todo congelado (cielo incluido)
	 *   StopFlash = 0..1 destello en negativo del impacto · StopStrength = 0..1 cuánto se nota todo.
	 * Lo que está dentro de la onda queda gris metálico e índigo; el frente de la onda, en negativo naranja; lo que tiene
	 * Custom Stencil 1 (el jugador) conserva su color.
	 */
	void CreateWorldStopMaterialIfMissing()
	{
		if (FPackageName::DoesPackageExist(WorldStopPackage))
		{
			return;
		}

		UPackage* Package = CreatePackage(WorldStopPackage);
		UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_TemporalWorldStop"), RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;

		UMaterialExpressionCustom* Custom = NewObject<UMaterialExpressionCustom>(Material);
		Custom->Material = Material;
		Custom->Description = TEXT("TemporalWorldStop");
		Custom->OutputType = CMOT_Float3;
		Custom->Code = TEXT(R"HLSL(
float2 ViewUV = GetViewportUV(Parameters);
float3 Col = SceneTextureLookup(ClampSceneTextureUV(ViewportUVToSceneTextureUV(ViewUV, 14), 14), 14, true).rgb;
float Strength = saturate(StopParams.w);
float Radius = StopParams.x;
float Fill = saturate(StopParams.y);
float Flash = saturate(StopParams.z);
float Dist = distance(WorldPos, StopCenter.xyz);
float Inside = max(1.0 - smoothstep(Radius - 60.0, Radius, Dist), Fill);
float Front = exp(-pow((Dist - Radius) / 140.0, 2.0)) * (1.0 - Fill);
float Lum = dot(Col, float3(0.299, 0.587, 0.114));
// Mundo congelado: gris metálico con tinte índigo, un poco más oscuro.
float3 Frozen = lerp(float3(Lum, Lum, Lum) * 0.9, float3(0.32, 0.34, 0.62) * (Lum * 1.7 + 0.04), 0.5);
// Impacto: negativo de foto con cielo naranja y amarillo (las sombras se vuelven blancas).
float3 Negative = (1.0 - Col) * float3(1.0, 0.78, 0.38);
float3 Out = lerp(Col, Frozen, Inside * Strength);
Out = lerp(Out, Negative, saturate((Front + Flash) * Strength));
// El jugador (Custom Stencil 1) conserva su color.
float Keep = Stencil.r > 0.5 ? 1.0 : 0.0;
return lerp(Out, Col, Keep);
)HLSL");

		auto AddVectorInput = [Material, Custom](const TCHAR* Name)
		{
			UMaterialExpressionVectorParameter* Param = NewObject<UMaterialExpressionVectorParameter>(Material);
			Param->Material = Material;
			Param->ParameterName = Name;
			Param->DefaultValue = FLinearColor(0.f, 0.f, 0.f, 0.f);
			Material->GetExpressionCollection().AddExpression(Param);
			FCustomInput Input;
			Input.InputName = Name;
			Input.Input.Connect(5, Param);
			Custom->Inputs.Add(Input);
		};
		Custom->Inputs.Reset();
		AddVectorInput(TEXT("StopCenter"));
		// x = radio, y = relleno, z = destello, w = fuerza.
		AddVectorInput(TEXT("StopParams"));

		UMaterialExpressionWorldPosition* World = NewObject<UMaterialExpressionWorldPosition>(Material);
		World->Material = Material;
		Material->GetExpressionCollection().AddExpression(World);
		FCustomInput WorldInput;
		WorldInput.InputName = TEXT("WorldPos");
		WorldInput.Input.Connect(0, World);
		Custom->Inputs.Add(WorldInput);

		UMaterialExpressionSceneTexture* StencilTexture = NewObject<UMaterialExpressionSceneTexture>(Material);
		StencilTexture->Material = Material;
		StencilTexture->SceneTextureId = PPI_CustomStencil;
		Material->GetExpressionCollection().AddExpression(StencilTexture);
		FCustomInput StencilInput;
		StencilInput.InputName = TEXT("Stencil");
		StencilInput.Input.Connect(0, StencilTexture);
		Custom->Inputs.Add(StencilInput);

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
		SaveNewMaterial(Package, Material, WorldStopPackage);
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
		CreateFieldMaterialIfMissing();
		CreateWorldStopMaterialIfMissing();
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

UMaterialInterface* TemporalFX::WorldStopMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, WorldStopObject, nullptr, LOAD_NoWarn);
	}
	return Cached.Get();
}

UMaterialInstanceDynamic* TemporalFX::MakeField(UObject* Outer, const FLinearColor& Color)
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, FieldObject, nullptr, LOAD_NoWarn);
	}
	if (!Cached.IsValid())
	{
		return MakeTranslucent(Outer, Color);
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Cached.Get(), Outer);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	return Material;
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
