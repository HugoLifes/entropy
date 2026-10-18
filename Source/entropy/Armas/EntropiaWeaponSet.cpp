// ENTROPÍA · Set de armas

#include "EntropiaWeaponSet.h"

#include "Engine/StaticMesh.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"
#endif

namespace EntropiaWeapons
{
	const TCHAR* SetObjectPath = TEXT("/Game/Entropia/Datos/DA_Armas.DA_Armas");
}

namespace EntropiaWeaponDefaults
{
	// Modelos de Fab importados por Tools/import-armas.ps1 (solo en la compu de Hugo).
	const TCHAR* Revolver = TEXT("/Game/Entropia/ArmasFab/Revolver/revolver/StaticMeshes/revolver.revolver");
	const TCHAR* RifleBody = TEXT("/Game/Entropia/ArmasFab/Fusil/fusil_6.fusil_6");
	const TCHAR* RifleScope = TEXT("/Game/Entropia/ArmasFab/Fusil/fusil_scope_6.fusil_scope_6");
	const TCHAR* RifleMagazine = TEXT("/Game/Entropia/ArmasFab/Fusil/fusil_charguer_6.fusil_charguer_6");
	const TCHAR* PistolBody = TEXT("/Game/Entropia/ArmasFab/Pistola/pistol_2.pistol_2");
	const TCHAR* PistolMagazine = TEXT("/Game/Entropia/ArmasFab/Pistola/pistol_charger_2.pistol_charger_2");

	/** Metal oscuro para quitarle el verde de juguete al pack low poly. */
	const FLinearColor DarkMetal(0.07f, 0.07f, 0.08f, 1.f);

	FEntropiaWeaponPart Part(const TCHAR* Path, const FTransform& Offset, bool bMagazine = false)
	{
		FEntropiaWeaponPart Result;
		Result.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Path));
		Result.Offset = Offset;
		Result.bMagazine = bMagazine;
		return Result;
	}

	// El hueso hand_r del maniquí, en la pose de apuntar: -X = hacia donde apunta el cañón (los dedos), +Z = arriba.
	// La empuñadura va en la palma, unos 8 cm delante de la muñeca.
	// Revólver (glb): cañón hacia -X, empuñadura en el origen, 49 cm de largo → al 80 %.
	const FTransform RevolverInHand(FRotator(0.f, 0.f, 0.f), FVector(-2.f, 0.f, 1.f), FVector(0.8f));
	// Pack low poly: cañón hacia -Y, unas 4 veces más grande que de verdad → girado -90°.
	// Fusil al 28 % (≈ 90 cm), empuñadura en Y≈25, Z≈-40 del modelo.
	const FTransform RifleInHand(FRotator(0.f, -90.f, 0.f), FVector(-15.f, 0.f, 8.f), FVector(0.28f));
	const FTransform PistolInHand(FRotator(0.f, 180.f, 0.f), FVector(0.f, 4.6f, 4.f), FVector(0.16f));
}

UEntropiaWeaponSet::UEntropiaWeaponSet()
{
	using EntropiaWeaponDefaults::Part;

	// Revólver "Aurora": 6 tiros, lento, pega muy fuerte y patea mucho.
	Revolver.Name = TEXT("Revolver");
	Revolver.DisplayName = NSLOCTEXT("Entropia", "Revolver", "REVÓLVER");
	Revolver.Parts.Add(Part(EntropiaWeaponDefaults::Revolver, EntropiaWeaponDefaults::RevolverInHand));
	Revolver.MuzzleOffset = FVector(-37.f, 0.f, 5.f);
	Revolver.FireInterval = 0.45f;
	Revolver.bAutomatic = false;
	Revolver.MagazineSize = 6;
	Revolver.ReloadSeconds = 1.6f;
	Revolver.TorsoDamage = 55.f;
	Revolver.HeadDamage = 200.f;
	Revolver.BulletSpeed = 8000.f;
	Revolver.Impulse = 900.f;
	Revolver.SpreadMin = 0.3f;
	Revolver.SpreadMax = 2.5f;
	Revolver.SpreadPerShot = 1.2f;
	Revolver.SpreadRecovery = 4.f;
	Revolver.RecoilPitch = 3.2f;
	Revolver.RecoilYaw = 0.6f;

	// Fusil: automático, 30 tiros, rápido, pega menos.
	Rifle.Name = TEXT("Rifle");
	Rifle.DisplayName = NSLOCTEXT("Entropia", "Rifle", "FUSIL");
	Rifle.Parts.Add(Part(EntropiaWeaponDefaults::RifleBody, EntropiaWeaponDefaults::RifleInHand));
	Rifle.Parts.Add(Part(EntropiaWeaponDefaults::RifleScope, EntropiaWeaponDefaults::RifleInHand));
	Rifle.Parts.Add(Part(EntropiaWeaponDefaults::RifleMagazine, EntropiaWeaponDefaults::RifleInHand, true));
	Rifle.MuzzleOffset = FVector(-66.f, 0.f, 8.f);
	Rifle.Tint = EntropiaWeaponDefaults::DarkMetal;
	Rifle.FireInterval = 0.1f;
	Rifle.bAutomatic = true;
	Rifle.MagazineSize = 30;
	Rifle.ReloadSeconds = 1.9f;
	Rifle.TorsoDamage = 20.f;
	Rifle.HeadDamage = 60.f;
	Rifle.BulletSpeed = 9000.f;
	Rifle.Impulse = 450.f;
	Rifle.SpreadMin = 0.8f;
	Rifle.SpreadMax = 3.f;
	Rifle.SpreadPerShot = 0.25f;
	Rifle.SpreadRecovery = 5.f;
	Rifle.RecoilPitch = 0.7f;
	Rifle.RecoilYaw = 0.35f;

	// La pistola de los dummies (solo el modelo cuenta).
	DummyPistol.Name = TEXT("DummyPistol");
	DummyPistol.DisplayName = NSLOCTEXT("Entropia", "DummyPistol", "PISTOLA");
	DummyPistol.Parts.Add(Part(EntropiaWeaponDefaults::PistolBody, EntropiaWeaponDefaults::PistolInHand));
	DummyPistol.Parts.Add(Part(EntropiaWeaponDefaults::PistolMagazine, EntropiaWeaponDefaults::PistolInHand, true));
	DummyPistol.MuzzleOffset = FVector(0.f, 17.f, 7.f);
	DummyPistol.Tint = EntropiaWeaponDefaults::DarkMetal;
}

const UEntropiaWeaponSet* UEntropiaWeaponSet::Get()
{
	static TWeakObjectPtr<const UEntropiaWeaponSet> Cached;
	if (!Cached.IsValid())
	{
		const UEntropiaWeaponSet* Asset = LoadObject<UEntropiaWeaponSet>(nullptr, EntropiaWeapons::SetObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		Cached = Asset ? Asset : GetDefault<UEntropiaWeaponSet>();
	}
	return Cached.Get();
}

#if WITH_EDITOR
namespace EntropiaWeaponSetEditor
{
	const TCHAR* SetPackage = TEXT("/Game/Entropia/Datos/DA_Armas");
	const TCHAR* FabFolder = TEXT("/Game/Entropia/ArmasFab");

	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}

	/** Crea DA_Armas con los valores por defecto, si todavía no existe (Hugo lo edita en el editor). */
	void CreateWeaponSetIfMissing()
	{
		if (FPackageName::DoesPackageExist(SetPackage))
		{
			return;
		}
		UPackage* Package = CreatePackage(SetPackage);
		UEntropiaWeaponSet* Set = NewObject<UEntropiaWeaponSet>(Package, TEXT("DA_Armas"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Set);
		if (SaveAsset(Set))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: creado %s"), SetPackage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ENTROPIA: no se pudo guardar %s"), SetPackage);
		}
	}

	/** Marca que deja Tools/import-armas.ps1: hay modelos recién importados que revisar. */
	FString PendingMarker()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ArmasFab.pendiente"));
	}

	/** Las texturas de 4K de Fab no caben en 4 GB de video: se limitan una vez (y se guardan así). Solo después de importar. */
	void LimitImportedTextures()
	{
		if (!FPaths::FileExists(PendingMarker()))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: ArmasFab sin cambios (no se revisa)"));
			return;
		}
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		Registry.ScanPathsSynchronous({ FString(FabFolder) }, /*bForceRescan=*/false);
		TArray<FAssetData> Assets;
		Registry.GetAssetsByPath(FName(FabFolder), Assets, /*bRecursive=*/true);
		const int32 Limit = UEntropiaWeaponSet::Get()->MaxTextureSize;
		for (const FAssetData& Data : Assets)
		{
			if (Data.IsInstanceOf(UStaticMesh::StaticClass()))
			{
				UStaticMesh* Mesh = Cast<UStaticMesh>(Data.GetAsset());
				if (Mesh && Mesh->IsNaniteEnabled())
				{
					// Armas chicas: sin Nanite (su material importado no lo soporta y saldría gris por defecto).
					FMeshNaniteSettings Settings = Mesh->GetNaniteSettings();
					Settings.bEnabled = false;
					Mesh->SetNaniteSettings(Settings);
					Mesh->PostEditChange();
					UE_LOG(LogTemp, Display, TEXT("ENTROPIA: sin Nanite: %s (%s)"), *Mesh->GetPathName(), SaveAsset(Mesh) ? TEXT("guardado") : TEXT("sin guardar"));
				}
				continue;
			}
			if (!Data.IsInstanceOf(UTexture::StaticClass()))
			{
				continue;
			}
			UTexture* Texture = Cast<UTexture>(Data.GetAsset());
			if (!Texture || (Texture->MaxTextureSize > 0 && Texture->MaxTextureSize <= Limit))
			{
				continue;
			}
			Texture->MaxTextureSize = Limit;
			Texture->PostEditChange();
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: textura limitada a %d: %s (%s)"), Limit, *Texture->GetPathName(),
				SaveAsset(Texture) ? TEXT("guardada") : TEXT("sin guardar"));
		}
		IFileManager::Get().Delete(*PendingMarker());
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA: ArmasFab revisado (texturas y Nanite)"));
	}

	FDelayedAutoRegisterHelper GCreateEntropiaWeaponSet(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (!IsRunningCommandlet())
		{
			CreateWeaponSetIfMissing();
			LimitImportedTextures();
		}
	});
}
#endif
