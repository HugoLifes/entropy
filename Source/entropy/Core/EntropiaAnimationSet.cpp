// ENTROPÍA · Set de animaciones

#include "EntropiaAnimationSet.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

namespace EntropiaAnim
{
	const TCHAR* SetObjectPath = TEXT("/Game/Entropia/Datos/DA_Animaciones.DA_Animaciones");
}

namespace EntropiaAnimDefaults
{
	// Animaciones de la plantilla de Epic (provisionales hasta tener las de verdad).
	const TCHAR* Land = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land.MM_Land");
	const TCHAR* Jump = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump");
	const TCHAR* WallJump = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_WallJump.MM_WallJump");
	const TCHAR* Dash = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash");
	const TCHAR* Attack1 = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01");
	const TCHAR* Attack2 = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02");
	const TCHAR* Attack3 = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03");
	const TCHAR* Charged = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack");
	const TCHAR* HitLight = TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01");
	const TCHAR* HitMedium = TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01.MM_HitReact_Front_Med_01");
	const TCHAR* HitHeavy = TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01");
	// "Death_Back" = le pegan por la espalda y cae hacia adelante (lejos de ti al romperle el cuello).
	const TCHAR* DeathBack = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01");
	const TCHAR* PistolAim = TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS");
	const TCHAR* PistolFire = TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire.MM_Pistol_Fire");
	const TCHAR* PistolReload = TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload.MM_Pistol_Reload");
	const TCHAR* RifleAim = TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS");
	const TCHAR* RifleReload = TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload");

	FEntropiaAnimSlot Make(const TCHAR* Path, float PlayRate = 1.f, float StartTime = 0.f, bool bLoop = false)
	{
		FEntropiaAnimSlot Slot;
		Slot.Anim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path));
		Slot.PlayRate = PlayRate;
		Slot.StartTime = StartTime;
		Slot.bLoop = bLoop;
		return Slot;
	}
}

UEntropiaAnimationSet::UEntropiaAnimationSet()
{
	using EntropiaAnimDefaults::Make;
	// Poses: PlayRate 0 congela la animación en StartTime (las rodillas dobladas del aterrizaje = agachado).
	CrouchPose = Make(EntropiaAnimDefaults::Land, 0.f, 0.12f, true);
	Roll = Make(EntropiaAnimDefaults::Land, 0.f, 0.12f, true);
	HardLanding = Make(EntropiaAnimDefaults::Land, 0.8f);

	DashVault = Make(EntropiaAnimDefaults::Jump, 1.3f);
	Kong = Make(EntropiaAnimDefaults::Dash, 1.1f);
	StepUp = Make(EntropiaAnimDefaults::Jump, 1.5f);
	Climb = Make(EntropiaAnimDefaults::WallJump, 1.f);
	WallRun = Make(EntropiaAnimDefaults::WallJump, 1.2f);
	Hang = Make(EntropiaAnimDefaults::WallJump, 0.f, 0.15f, true);
	ClimbFromHang = Make(EntropiaAnimDefaults::WallJump, 1.f, 0.15f);

	CoverLow = Make(EntropiaAnimDefaults::Land, 0.f, 0.12f, true);
	// Cobertura alta: de pie (sin animación especial por ahora).

	Punch1 = Make(EntropiaAnimDefaults::Attack1, 1.3f);
	Punch2 = Make(EntropiaAnimDefaults::Attack2, 1.3f);
	Punch3 = Make(EntropiaAnimDefaults::Attack3, 1.2f);
	NeckBreak = Make(EntropiaAnimDefaults::Attack2, 1.5f);
	FocusStrike = Make(EntropiaAnimDefaults::Charged, 1.4f);
	Grab = Make(EntropiaAnimDefaults::Attack1, 0.f, 0.25f, true);

	VictimHit = Make(EntropiaAnimDefaults::HitLight, 1.2f);
	VictimNeckBreak = Make(EntropiaAnimDefaults::DeathBack, 1.f);
	VictimSubdued = Make(EntropiaAnimDefaults::HitHeavy, 0.f, 0.35f, true);
	VictimDisarmed = Make(EntropiaAnimDefaults::HitMedium, 1.f);
	DummyAim = Make(EntropiaAnimDefaults::PistolAim, 1.f, 0.f, true);
	DummyFire = Make(EntropiaAnimDefaults::PistolFire, 1.f);

	RevolverAim = Make(EntropiaAnimDefaults::PistolAim, 1.f, 0.f, true);
	// Disparar: vacío por ahora (las de la plantilla son solo para la parte de arriba del cuerpo y, de cuerpo entero,
	// bajan los brazos). El retroceso de la cámara da el golpe. Aquí van las de Mixamo en el pulido.
	RevolverReload = Make(EntropiaAnimDefaults::PistolReload, 1.f);
	RifleAim = Make(EntropiaAnimDefaults::RifleAim, 1.f, 0.f, true);
	RifleReload = Make(EntropiaAnimDefaults::RifleReload, 1.f);
}

const UEntropiaAnimationSet* UEntropiaAnimationSet::Get()
{
	static TWeakObjectPtr<const UEntropiaAnimationSet> Cached;
	if (!Cached.IsValid())
	{
		const UEntropiaAnimationSet* Asset = LoadObject<UEntropiaAnimationSet>(nullptr, EntropiaAnim::SetObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		Cached = Asset ? Asset : GetDefault<UEntropiaAnimationSet>();
	}
	return Cached.Get();
}

namespace EntropiaAnim
{
	UAnimInstance* AnimOf(const ACharacter* Character)
	{
		const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
		return Mesh ? Mesh->GetAnimInstance() : nullptr;
	}

	float Play(ACharacter* Character, const FEntropiaAnimSlot& Slot)
	{
		UAnimInstance* Anim = AnimOf(Character);
		UAnimSequenceBase* Asset = Slot.Anim.LoadSynchronous();
		if (!Anim || !Asset)
		{
			return 0.f;
		}
		// El C++ mueve al personaje: la animación nunca debe moverlo.
		Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		if (UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			const float Length = Anim->Montage_Play(Montage, FMath::Max(Slot.PlayRate, 0.0001f), EMontagePlayReturnType::Duration, Slot.StartTime);
			if (Slot.PlayRate <= 0.f)
			{
				Anim->Montage_SetPlayRate(Montage, 0.f);
			}
			return Length;
		}
		const int32 LoopCount = Slot.bLoop ? 100000 : 1;
		UAnimMontage* Dynamic = Anim->PlaySlotAnimationAsDynamicMontage(Asset, TEXT("DefaultSlot"), Slot.BlendIn, Slot.BlendOut,
			FMath::Max(Slot.PlayRate, 0.0001f), LoopCount, -1.f, Slot.StartTime);
		if (Dynamic && Slot.PlayRate <= 0.f)
		{
			Anim->Montage_SetPlayRate(Dynamic, 0.f);
		}
		return Slot.PlayRate > 0.f ? FMath::Max(Asset->GetPlayLength() - Slot.StartTime, 0.f) / Slot.PlayRate : 0.f;
	}

	void Stop(ACharacter* Character, float BlendOut)
	{
		if (UAnimInstance* Anim = AnimOf(Character))
		{
			Anim->StopSlotAnimation(BlendOut, TEXT("DefaultSlot"));
			Anim->Montage_Stop(BlendOut);
		}
	}

	bool IsPlaying(const ACharacter* Character, const FEntropiaAnimSlot& Slot)
	{
		const UAnimInstance* Anim = AnimOf(Character);
		const UAnimSequenceBase* Asset = Slot.Anim.Get();
		if (!Anim || !Asset)
		{
			return false;
		}
		if (const UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			return Anim->Montage_IsPlaying(Montage);
		}
		return Anim->IsPlayingSlotAnimation(Asset, TEXT("DefaultSlot"));
	}
}

#if WITH_EDITOR
namespace EntropiaAnimSetEditor
{
	const TCHAR* SetPackage = TEXT("/Game/Entropia/Datos/DA_Animaciones");

	/** Crea DA_Animaciones con las animaciones provisionales, si todavía no existe (Hugo lo edita en el editor). */
	void CreateAnimationSetIfMissing()
	{
		if (FPackageName::DoesPackageExist(SetPackage))
		{
			return;
		}
		UPackage* Package = CreatePackage(SetPackage);
		UEntropiaAnimationSet* Set = NewObject<UEntropiaAnimationSet>(Package, TEXT("DA_Animaciones"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Set);
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(SetPackage, FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(Package, Set, *Filename, Args))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA: creado %s"), SetPackage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ENTROPIA: no se pudo guardar %s"), SetPackage);
		}
	}

	FDelayedAutoRegisterHelper GCreateEntropiaAnimationSet(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (!IsRunningCommandlet())
		{
			CreateAnimationSetIfMissing();
		}
	});
}
#endif
