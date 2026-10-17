// ENTROPÍA · El arma del jugador

#include "EntropiaWeaponComponent.h"
#include "EntropiaWeaponSet.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaCoverComponent.h"
#include "EntropiaGrabComponent.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaParkourComponent.h"
#include "InverseStepComponent.h"
#include "TakedownComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalArmedDummy.h"
#include "TemporalDilation.h"
#include "TemporalFX.h"
#include "TemporalProjectile.h"
#include "TemporalWorldStopComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace EntropiaWeaponLook
{
	/** El maniquí no trae el socket weapon_r: las armas van en el hueso de la mano. */
	const FName HandSocket(TEXT("hand_r"));
	const FLinearColor MuzzleFlash(1.f, 0.78f, 0.3f);
	/** Si un modelo no está importado: cubo gris del tamaño aproximado del arma. */
	const FLinearColor Placeholder(0.12f, 0.12f, 0.13f);
}

UEntropiaWeaponComponent::UEntropiaWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEntropiaWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	Aim = GetOwner()->FindComponentByClass<UTemporalAimComponent>();
	for (int32 i = 0; i < NumWeapons; ++i)
	{
		States[i].Init(StatsOf(i));
	}
	BuildVisual();
}

void UEntropiaWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopKillCam();
	Super::EndPlay(EndPlayReason);
}

const FEntropiaWeaponStats& UEntropiaWeaponComponent::StatsOf(int32 Index) const
{
	const UEntropiaWeaponSet* Set = UEntropiaWeaponSet::Get();
	return Index == 1 ? Set->Rifle : Set->Revolver;
}

void UEntropiaWeaponComponent::BuildVisual()
{
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Parts.Reset();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body)
	{
		return;
	}
	const FEntropiaWeaponStats& Stats = CurrentStats();
	for (const FEntropiaWeaponPart& Def : Stats.Parts)
	{
		UStaticMesh* Mesh = Def.Mesh.LoadSynchronous();
		const bool bPlaceholder = Mesh == nullptr;
		if (bPlaceholder)
		{
			// Sin el modelo de Fab (otra compu): un cubo y ya, aunque el arma tenga varias piezas.
			if (Parts.Num() > 0)
			{
				break;
			}
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Character);
		Part->SetupAttachment(Body, EntropiaWeaponLook::HandSocket);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetStaticMesh(Mesh);
		// Stencil 1 = el mundo detenido lo deja a color (como a ti).
		Part->SetRenderCustomDepth(true);
		Part->SetCustomDepthStencilValue(1);
		if (bPlaceholder)
		{
			const float Length = Current == 1 ? 0.7f : 0.3f;
			Part->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector(-8.f - Length * 50.f, 0.f, 3.f), FVector(Length, 0.06f, 0.12f)));
		}
		else
		{
			Part->SetRelativeTransform(Def.Offset);
		}
		Part->RegisterComponent();
		if (bPlaceholder || Stats.Tint.A > 0.f)
		{
			TemporalFX::ApplyToAllSlots(Part, TemporalFX::MakeSolid(this, bPlaceholder ? EntropiaWeaponLook::Placeholder : Stats.Tint));
		}
		Parts.Add(Part);
	}
}

FVector UEntropiaWeaponComponent::GetMuzzleLocation() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || !Body->DoesSocketExist(EntropiaWeaponLook::HandSocket))
	{
		return GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 50.f);
	}
	return Body->GetSocketTransform(EntropiaWeaponLook::HandSocket).TransformPosition(CurrentStats().MuzzleOffset);
}

FVector UEntropiaWeaponComponent::FindAimPoint(FVector& OutViewDirection) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	FVector ViewLocation = GetOwner()->GetActorLocation();
	FRotator ViewRotation = GetOwner()->GetActorRotation();
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	OutViewDirection = ViewRotation.Vector();
	const FVector End = ViewLocation + OutViewDirection * 10000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaWeaponAim), false, GetOwner());
	FVector Point = End;
	float Best = 10000.f;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, Params))
	{
		Point = Hit.ImpactPoint;
		Best = Hit.Distance;
	}
	// Las cápsulas de los maniquíes no bloquean la visibilidad: se buscan aparte.
	if (GetWorld()->LineTraceSingleByObjectType(Hit, ViewLocation, End, FCollisionObjectQueryParams(ECC_Pawn), Params) && Hit.Distance < Best)
	{
		Point = Hit.ImpactPoint;
	}
	return Point;
}

void UEntropiaWeaponComponent::TriggerPressed()
{
	bTriggerHeld = true;
	FireOnce(true);
}

void UEntropiaWeaponComponent::TriggerReleased()
{
	bTriggerHeld = false;
}

bool UEntropiaWeaponComponent::FireOnce(bool bJustPressed)
{
	if (!Aim.IsValid() || !Aim->IsAiming())
	{
		return false;
	}
	FEntropiaWeaponState& State = States[Current];
	const FEntropiaWeaponStats& Stats = CurrentStats();
	if (State.Ammo <= 0 && !State.IsReloading())
	{
		// Clic con el arma vacía: recarga sola.
		if (bJustPressed)
		{
			Reload();
		}
		return false;
	}
	const bool bMoving = GetOwner()->GetVelocity().Size2D() > 50.f;
	const float SpreadHalfAngle = FMath::DegreesToRadians(State.SpreadDegrees(Stats, bMoving) * 0.5f);
	if (!State.TryFire(Stats, bTriggerHeld, bJustPressed))
	{
		return false;
	}

	FVector ViewDirection;
	const FVector AimPoint = FindAimPoint(ViewDirection);
	FVector Muzzle = GetMuzzleLocation();
	// Si el cañón quedó dentro de una pared (pegado a una cobertura), la bala sale de este lado.
	{
		const FVector From = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, Muzzle.Z - GetOwner()->GetActorLocation().Z);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaMuzzle), false, GetOwner());
		FHitResult Blocked;
		if (GetWorld()->LineTraceSingleByChannel(Blocked, From, Muzzle, ECC_Visibility, Params))
		{
			Muzzle = Blocked.ImpactPoint - (Muzzle - From).GetSafeNormal() * 5.f;
		}
	}
	FVector Direction = (AimPoint - Muzzle).GetSafeNormal();
	if (Direction.IsNearlyZero() || FVector::DotProduct(Direction, ViewDirection) < 0.2f)
	{
		Direction = ViewDirection;
	}
	Direction = FMath::VRandCone(Direction, SpreadHalfAngle);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATemporalProjectile* Bullet = GetWorld()->SpawnActor<ATemporalProjectile>(ATemporalProjectile::StaticClass(), Muzzle, Direction.Rotation(), SpawnParams))
	{
		Bullet->Speed = Stats.BulletSpeed;
		Bullet->Range = 9000.f;
		Bullet->bFromPlayer = true;
		Bullet->WeaponIndex = Current;
		Bullet->TorsoDamage = Stats.TorsoDamage;
		Bullet->HeadDamage = Stats.HeadDamage;
		Bullet->Impulse = Stats.Impulse;
		TWeakObjectPtr<UEntropiaWeaponComponent> WeakThis(this);
		Bullet->OnEvent = [WeakThis](ATemporalProjectile* Shot, EEntropiaBulletEvent Event, AActor* Victim)
		{
			if (UEntropiaWeaponComponent* Self = WeakThis.Get())
			{
				Self->HandleBulletEvent(Shot, Event, Victim);
			}
		};
		Bullet->Launch(Muzzle, Direction, GetOwner());

		// Tope de balas vivas (rendimiento): desaparece la más vieja.
		LiveBullets.RemoveAll([](const TWeakObjectPtr<ATemporalProjectile>& Live) { return !Live.IsValid(); });
		LiveBullets.Add(Bullet);
		for (int32 Evict = EntropiaGuns::OldestToEvict(LiveBullets.Num(), EntropiaGuns::MaxLiveBullets); Evict > 0; --Evict)
		{
			if (ATemporalProjectile* Oldest = LiveBullets[0].Get())
			{
				Oldest->Destroy();
			}
			LiveBullets.RemoveAt(0);
		}
	}
	++TotalShots;
	// Fogonazo: chico, y en el fusil uno de cada 3 tiros (el estallido del motor sale blanco y a 10 tiros/s tapa todo).
	if (Current == 0 || TotalShots % 3 == 1)
	{
		TemporalFX::Burst(this, Muzzle, EntropiaWeaponLook::MuzzleFlash, Current == 0 ? 0.08f : 0.05f);
	}

	// Retroceso: la cámara patea hacia arriba y un poco de lado; luego regresa sola.
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = Pawn->GetController())
		{
			FRotator Rotation = Controller->GetControlRotation();
			Rotation.Pitch += Stats.RecoilPitch;
			Rotation.Yaw += FMath::FRandRange(-Stats.RecoilYaw, Stats.RecoilYaw);
			Controller->SetControlRotation(Rotation);
			RecoilDebt += Stats.RecoilPitch;
		}
	}
	if (!Stats.bAutomatic)
	{
		PlayOneShot(EPose::Fire);
	}
	return true;
}

bool UEntropiaWeaponComponent::Reload()
{
	if (!States[Current].StartReload(CurrentStats()))
	{
		return false;
	}
	PlayOneShot(EPose::Reload);
	return true;
}

void UEntropiaWeaponComponent::CycleWeapon(int32 Direction)
{
	States[Current].CancelReload();
	StopOwnPose();
	Current = ((Current + Direction) % NumWeapons + NumWeapons) % NumWeapons;
	BuildVisual();
}

float UEntropiaWeaponComponent::GetSpreadDegrees() const
{
	return CurrentState().SpreadDegrees(CurrentStats(), GetOwner() && GetOwner()->GetVelocity().Size2D() > 50.f);
}

float UEntropiaWeaponComponent::GetHitMarkerAlpha() const
{
	return FMath::Clamp(HitMarkerLeft / 0.25f, 0.f, 1.f);
}

float UEntropiaWeaponComponent::GetRefundAlpha() const
{
	return FMath::Clamp(RefundLeft / 0.6f, 0.f, 1.f);
}

int32 UEntropiaWeaponComponent::NumLiveBullets() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ATemporalProjectile>& Live : LiveBullets)
	{
		Count += Live.IsValid() ? 1 : 0;
	}
	return Count;
}

void UEntropiaWeaponComponent::HandleBulletEvent(ATemporalProjectile* Bullet, EEntropiaBulletEvent Event, AActor* Victim)
{
	switch (Event)
	{
	case EEntropiaBulletEvent::Hit:
	case EEntropiaBulletEvent::Stored:
		HitMarkerLeft = 0.25f;
		bMarkerKill = false;
		break;
	case EEntropiaBulletEvent::KnockedDown:
		HitMarkerLeft = 0.25f;
		bMarkerKill = true;
		if (IsLastStanding(Victim))
		{
			StartKillCam();
		}
		break;
	case EEntropiaBulletEvent::Returned:
	{
		// Recarga inversa: la bala vuelve al cargador de SU arma (nunca pasa del máximo).
		const int32 Index = Bullet && Bullet->WeaponIndex >= 0 && Bullet->WeaponIndex < NumWeapons ? Bullet->WeaponIndex : Current;
		if (States[Index].Refund(StatsOf(Index), 1) > 0)
		{
			RefundLeft = 0.6f;
		}
		break;
	}
	}
}

bool UEntropiaWeaponComponent::IsLastStanding(const AActor* Victim) const
{
	const ATemporalArmedDummy* Fallen = Cast<ATemporalArmedDummy>(Victim);
	if (!Fallen)
	{
		return false;
	}
	// Solo cuentan los de la misma sala (la sala es su dueña).
	const AActor* Room = Fallen->GetOwner();
	for (TActorIterator<ATemporalArmedDummy> It(GetWorld()); It; ++It)
	{
		if (*It == Fallen || It->GetOwner() != Room)
		{
			continue;
		}
		if (It->IsArmed() && !It->IsKnockedDown() && !It->HasPendingHit() && !It->IsCollapsing())
		{
			return false;
		}
	}
	return true;
}

void UEntropiaWeaponComponent::StartKillCam()
{
	const UTemporalWorldStopComponent* WorldStop = GetOwner()->FindComponentByClass<UTemporalWorldStopComponent>();
	if (WorldStop && WorldStop->IsActive())
	{
		return;
	}
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->RequestGlobal(this, KillCamDilation);
	}
	KillCamLeft = KillCamSeconds;
	if (Aim.IsValid())
	{
		Aim->StartFocus(0.9f);
	}
}

void UEntropiaWeaponComponent::StopKillCam()
{
	if (KillCamLeft <= 0.f)
	{
		return;
	}
	KillCamLeft = 0.f;
	if (UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this))
	{
		Dilation->ReleaseGlobal(this);
	}
}

void UEntropiaWeaponComponent::CancelAll()
{
	bTriggerHeld = false;
	for (FEntropiaWeaponState& State : States)
	{
		State.CancelReload();
	}
	StopKillCam();
	StopOwnPose();
	RecoilDebt = 0.f;
}

bool UEntropiaWeaponComponent::WantsPose() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Aim.IsValid() || !Aim->IsAiming() || Character->GetVelocity().Size2D() > 10.f
		|| (Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling()))
	{
		return false;
	}
	// Las demás acciones tienen su propia animación de cuerpo entero: el arma no se la quita.
	const UEntropiaCoverComponent* Cover = Character->FindComponentByClass<UEntropiaCoverComponent>();
	const UEntropiaLocomotionComponent* Locomotion = Character->FindComponentByClass<UEntropiaLocomotionComponent>();
	const UEntropiaParkourComponent* Parkour = Character->FindComponentByClass<UEntropiaParkourComponent>();
	const UEntropiaGrabComponent* Grab = Character->FindComponentByClass<UEntropiaGrabComponent>();
	const UTakedownComponent* Takedown = Character->FindComponentByClass<UTakedownComponent>();
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	return !(Cover && Cover->IsInCover()) && !(Locomotion && (Locomotion->IsCrouched() || Locomotion->IsRolling() || Locomotion->IsHardLanding()))
		&& !(Parkour && Parkour->IsBusy()) && !(Grab && Grab->IsHolding()) && !(Takedown && Takedown->IsBusy()) && !(Inverse && Inverse->IsInverseActive());
}

const FEntropiaAnimSlot* UEntropiaWeaponComponent::PoseSlot(EPose Pose, int32 Weapon) const
{
	const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
	if (!Set)
	{
		return nullptr;
	}
	const bool bRifle = Weapon == 1;
	switch (Pose)
	{
	case EPose::Aim: return bRifle ? &Set->RifleAim : &Set->RevolverAim;
	case EPose::Fire: return bRifle ? &Set->RifleFire : &Set->RevolverFire;
	default: return bRifle ? &Set->RifleReload : &Set->RevolverReload;
	}
}

void UEntropiaWeaponComponent::PlayOneShot(EPose Pose)
{
	if (!WantsPose())
	{
		return;
	}
	const FEntropiaAnimSlot* Slot = PoseSlot(Pose, Current);
	const float Seconds = Slot ? EntropiaAnim::Play(Cast<ACharacter>(GetOwner()), *Slot) : 0.f;
	if (Seconds > 0.f)
	{
		OneShotLeft = Seconds;
		bAimPosePlaying = false;
	}
}

void UEntropiaWeaponComponent::StopOwnPose()
{
	// Solo quita SUS animaciones (si otra acción ya las reemplazó, no la corta).
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	bool bMine = false;
	for (int32 Weapon = 0; Weapon < NumWeapons && Character; ++Weapon)
	{
		for (EPose Pose : { EPose::Aim, EPose::Fire, EPose::Reload })
		{
			const FEntropiaAnimSlot* Slot = PoseSlot(Pose, Weapon);
			bMine |= Slot && EntropiaAnim::IsPlaying(Character, *Slot);
		}
	}
	if (bMine)
	{
		EntropiaAnim::Stop(Character, 0.2f);
	}
	bAimPosePlaying = false;
	OneShotLeft = 0.f;
}

void UEntropiaWeaponComponent::UpdatePose(float DeltaTime)
{
	OneShotLeft = FMath::Max(0.f, OneShotLeft - DeltaTime);
	if (WantsPose())
	{
		if (!bAimPosePlaying && OneShotLeft <= 0.f)
		{
			if (const FEntropiaAnimSlot* Slot = PoseSlot(EPose::Aim, Current))
			{
				EntropiaAnim::Play(Cast<ACharacter>(GetOwner()), *Slot);
			}
			bAimPosePlaying = true;
		}
	}
	else if (bAimPosePlaying || OneShotLeft > 0.f)
	{
		StopOwnPose();
	}
}

void UEntropiaWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// DeltaTime = tu tiempo (el árbitro te compensa en los tiempos lentos del mundo).
	for (int32 i = 0; i < NumWeapons; ++i)
	{
		States[i].Tick(StatsOf(i), DeltaTime);
	}
	if (bTriggerHeld && CurrentStats().bAutomatic)
	{
		FireOnce(false);
	}

	// El retroceso regresa poco a poco.
	if (RecoilDebt > 0.f)
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (AController* Controller = Pawn ? Pawn->GetController() : nullptr)
		{
			const float Step = FMath::Min(RecoilDebt, RecoilRecovery * DeltaTime);
			FRotator Rotation = Controller->GetControlRotation();
			Rotation.Pitch -= Step;
			Controller->SetControlRotation(Rotation);
			RecoilDebt -= Step;
		}
		else
		{
			RecoilDebt = 0.f;
		}
	}

	// Los avisos y la cámara lenta cuentan en tiempo real.
	const float Real = TemporalFX::RealDelta();
	HitMarkerLeft = FMath::Max(0.f, HitMarkerLeft - Real);
	RefundLeft = FMath::Max(0.f, RefundLeft - Real);
	if (KillCamLeft > 0.f)
	{
		KillCamLeft -= Real;
		if (KillCamLeft <= 0.f)
		{
			KillCamLeft = 0.01f;
			StopKillCam();
		}
	}
	UpdatePose(DeltaTime);
}
