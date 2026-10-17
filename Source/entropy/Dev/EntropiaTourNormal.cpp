// ENTROPÍA · Modo tour normal: recorre las salas y prueba cada poder (capturas + FPS por paso).

#include "EntropiaTourComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaParkourComponent.h"
#include "EntropiaCoverComponent.h"
#include "TemporalCombatRoom.h"
#include "TemporalArmedDummy.h"
#include "EntropiaMeleeComponent.h"
#include "EntropiaGrabComponent.h"
#include "TemporalWorldStopComponent.h"
#include "TemporalDilation.h"
#include "TemporalParkourRoom.h"
#include "EntropiaPlayerPowers.h"
#include "EntropiaRoomsComponent.h"
#include "TemporalFXSubsystem.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"
#include "TemporalEnergyComponent.h"
#include "TemporalHUDWidget.h"
#include "ObjectInversionComponent.h"
#include "OverdriveComponent.h"
#include "TemporalAimComponent.h"
#include "TimeStopComponent.h"
#include "TemporalDashComponent.h"
#include "TemporalHealthComponent.h"
#include "TemporalReflexRoom.h"
#include "TemporalDilation.h"
#include "SlowEyeComponent.h"
#include "TakedownComponent.h"
#include "TemporalMannequin.h"
#include "InverseStepComponent.h"
#include "EchoGhost.h"
#include "TimeBubble.h"
#include "PressurePlateDoor.h"
#include "TemporalTestChamber.h"
#include "CrumblePlatform.h"
#include "TemporalFX.h"
#include "TemporalBreakable.h"
#include "TemporalPhysicsLab.h"
#include "TemporalTurret.h"
#include "TemporalProjectile.h"
#include "EntropiaWeaponComponent.h"
#include "EntropiaWeaponSet.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GenericPlatform/GenericPlatformMisc.h"

void UEntropiaTourComponent::BuildNormalSteps()
{
	auto Add = [this](const TCHAR* Name, float Seconds, TFunction<void()> Action)
	{
		TourSteps.Add({ Name, MoveTemp(Action), Seconds });
	};

	// Bala más reciente en el mundo (la que acaba de disparar la torreta).
	auto NewestBullet = [this]() -> ATemporalProjectile*
	{
		ATemporalProjectile* Best = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			if (!Best || It->GetGameTimeSinceCreation() < Best->GetGameTimeSinceCreation())
			{
				Best = *It;
			}
		}
		return Best;
	};
	auto Player = [this]() { return Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); };
	auto PlaceInLab = [this, Player](const FVector& Local)
	{
		if (Rooms->PhysicsLab && Player())
		{
			Player()->SetActorLocation(Rooms->PhysicsLab->GetActorTransform().TransformPosition(Local), false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* Controller = Player()->GetController())
			{
				Controller->SetControlRotation(FRotator(-10.f, Rooms->PhysicsLab->GetActorRotation().Yaw, 0.f));
			}
		}
	};

	Add(TEXT("inicio"), 2.5f, [] {});
	Add(TEXT("sala4"), 2.f, [this] { Rooms->GoToRoom(4); });
	// Fase 3: apuntar a la caja de la línea de tiro (cámara al hombro, la mira se pone ámbar y la caja se resalta).
	Add(TEXT("apuntar"), 1.6f, [this, Player]
	{
		ACharacter* Character = Player();
		AController* Controller = Character ? Character->GetController() : nullptr;
		if (Rooms->PhysicsLab && Controller)
		{
			const FVector Crate = Rooms->PhysicsLab->GetActorTransform().TransformPosition(FVector(800.f, 0.f, 50.f));
			Controller->SetControlRotation((Crate - Character->GetActorLocation() - FVector(0.f, 0.f, 60.f)).Rotation());
		}
		if (Powers->Aim)
		{
			Powers->Aim->SetAiming(true);
		}
	});
	// Fase 3 · Dash: uno en el piso (la captura sale a medio dash, con estela) y uno en el aire.
	Add(TEXT("dash"), 0.25f, [this]
	{
		if (Powers->Aim)
		{
			Powers->Aim->SetAiming(false);
		}
		const bool bOk = Powers->Dash && Powers->Dash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("salto"), 0.7f, [Player] { if (ACharacter* Character = Player()) { Character->Jump(); } });
	Add(TEXT("dash_aire"), 0.8f, [this, Player]
	{
		const bool bFalling = Player() && Player()->GetCharacterMovement()->IsFalling();
		const bool bOk = Powers->Dash && Powers->Dash->TryDash();
		const bool bSecond = Powers->Dash && Powers->Dash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash_aire=%d en_aire=%d segundo=%d"), bOk ? 1 : 0, bFalling ? 1 : 0, bSecond ? 1 : 0);
	});
	// Fase 3b · Correr (Shift mantenido) y agacharse (C).
	Add(TEXT("correr"), 1.2f, [this, Player]
	{
		if (Powers->Locomotion && Player())
		{
			Powers->Locomotion->SetSprint(true);
			Powers->Locomotion->SetTourInput(-Player()->GetActorRightVector());
		}
	});
	Add(TEXT("correr_revisa"), 0.3f, [this, Player]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR correr velocidad=%.0f"), Player() ? Player()->GetVelocity().Size2D() : -1.f);
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetSprint(false);
			Powers->Locomotion->SetTourInput(FVector::ZeroVector);
		}
	});
	Add(TEXT("agacharse"), 1.2f, [this, Player]
	{
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetCrouched(true);
		}
	});
	Add(TEXT("agachado_revisa"), 0.3f, [this, Player]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR agachado=%d media_altura=%.0f"),
			Powers->Locomotion && Powers->Locomotion->IsCrouched() ? 1 : 0,
			Player() ? Player()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : -1.f);
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetCrouched(false);
		}
	});
	// Fase 3b · Caídas: de 6 m corriendo (rueda), de 6 m quieto (aterrizaje duro), de 12 m (duro y daño).
	// Desde el centro de la sala 4, hacia el fondo (sin paredes cerca donde aterrizar).
	auto DropFrom = [this, Player](float Height, float Speed)
	{
		ACharacter* Character = Player();
		if (!Character || !Rooms->PhysicsLab)
		{
			return;
		}
		const FVector Spot = Rooms->PhysicsLab->GetActorTransform().TransformPosition(FVector(500.f, 300.f, 120.f));
		Character->SetActorLocation(Spot + FVector(0.f, 0.f, Height), false, nullptr, ETeleportType::TeleportPhysics);
		Character->SetActorRotation(FRotator(0.f, Rooms->PhysicsLab->GetActorRotation().Yaw, 0.f));
		if (Powers->Locomotion)
		{
			Powers->Locomotion->ResetFall();
		}
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		Character->GetCharacterMovement()->Velocity = Rooms->PhysicsLab->GetActorForwardVector() * Speed;
	};
	auto LogLanding = [this](const TCHAR* Name)
	{
		const FEntropiaLandingInfo Info = Powers->Locomotion ? Powers->Locomotion->GetLastLanding() : FEntropiaLandingInfo();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR %s tipo=%d altura=%.0f dano=%.0f vida=%.0f"), Name, static_cast<int32>(Info.Kind),
			Info.Height, Info.Damage, Powers->Health ? Powers->Health->Health : -1.f);
	};
	// Corriendo: el jugador mantiene la dirección (si no, el aire lo frena y cae quieto).
	Add(TEXT("caida_rodando"), 2.2f, [this, DropFrom, Player]
	{
		DropFrom(600.f, 450.f);
		if (Powers->Locomotion && Rooms->PhysicsLab)
		{
			Powers->Locomotion->SetTourInput(Rooms->PhysicsLab->GetActorForwardVector());
		}
	});
	Add(TEXT("caida_dura"), 1.6f, [this, DropFrom, LogLanding]
	{
		LogLanding(TEXT("caida_rodando"));
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetTourInput(FVector::ZeroVector);
		}
		DropFrom(600.f, 0.f);
	});
	Add(TEXT("caida_alta"), 1.8f, [DropFrom, LogLanding] { LogLanding(TEXT("caida_dura")); DropFrom(1200.f, 0.f); });
	Add(TEXT("caidas_revisa"), 0.3f, [this, LogLanding]
	{
		LogLanding(TEXT("caida_alta"));
		if (Powers->Health)
		{
			Powers->Health->ResetHealth();
		}
	});
	Add(TEXT("disparo"), 0.9f, [this] { Rooms->Fire(); });
	Add(TEXT("escombros"), 1.4f, [] {});
	Add(TEXT("invertir_bala"), 2.6f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && Powers->Inversion)
		{
			Powers->Inversion->BeginInversionOn(Bullet->Recorder);
		}
	});
	Add(TEXT("rearmado"), 1.6f, [this] { if (Powers->Inversion) { Powers->Inversion->EndInversion(); } });
	// Fase 3 · Detener: la bala se queda congelada a medio vuelo (después de romper la caja, antes del muro).
	Add(TEXT("disparo_para_detener"), 0.3f, [this] { Rooms->Fire(); });
	Add(TEXT("detener_bala"), 2.f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && Powers->TimeStop)
		{
			Powers->TimeStop->StopActor(Bullet);
		}
	});
	Add(TEXT("detener_suelta"), 1.2f, [this, NewestBullet]
	{
		if (ATemporalProjectile* Bullet = NewestBullet(); Bullet && Powers->TimeStop)
		{
			Powers->TimeStop->ReleaseActor(Bullet);
		}
	});
	Add(TEXT("romper_caja"), 1.4f, [this]
	{
		// Una caja que no esté en la línea de tiro.
		for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
		{
			const ATemporalTurret* Turret = Rooms->PhysicsLab ? Rooms->PhysicsLab->GetTurret() : nullptr;
			const FVector From = Turret ? Turret->GetActorLocation() : FVector::ZeroVector;
			const FVector To = Turret ? From + Turret->GetActorForwardVector() * 4000.f : FVector::ZeroVector;
			if (!It->IsBroken() && Turret && !It->SegmentCrosses(From + FVector(0, 0, 60), To + FVector(0, 0, 60)))
			{
				It->Break(It->GetActorLocation() - FVector(0, 0, 30), FVector(0, 0, -1));
				break;
			}
		}
	});
	Add(TEXT("shift_r"), 2.4f, [this] { Rooms->RewindAll(true); });
	Add(TEXT("shift_r_suelta"), 1.f, [this] { Rooms->RewindAll(false); });
	Add(TEXT("sobremarcha"), 2.6f, [this]
	{
		if (Powers->Overdrive)
		{
			Powers->Overdrive->StartOverdrive();
		}
		Rooms->Fire();
	});
	Add(TEXT("sobremarcha_off"), 1.2f, [this] { if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); } });
	Add(TEXT("burbuja"), 2.4f, [this, PlaceInLab]
	{
		PlaceInLab(FVector(350.f, -150.f, 120.f));
		Powers->SpawnBubble();
		Rooms->Fire();
	});
	// Fase 3 · Sala 5: las torretas te siguen y disparan; morir te regresa a la entrada con la vida llena.
	Add(TEXT("sala5"), 3.f, [this] { Rooms->GoToRoom(5); });
	Add(TEXT("recibir_balas"), 2.5f, [this, Player]
	{
		// Un paso al frente, a la vista de las tres torretas.
		if (Rooms->ReflexRoom && Player())
		{
			Player()->SetActorLocation(Rooms->ReflexRoom->RoomToWorld(FVector(700.f, 0.f, 120.f)), false, nullptr, ETeleportType::TeleportPhysics);
		}
	});
	Add(TEXT("muerte_reaparece"), 1.5f, [this]
	{
		const float Before = Powers->Health ? Powers->Health->Health : -1.f;
		if (Powers->Health)
		{
			Powers->Health->ApplyDamage(Powers->Health->Health);
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR vida_antes=%.0f"), Before);
	});
	Add(TEXT("reaparecio"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR reaparecio vida=%.0f muerto=%d global=%.2f jugador=%.2f"),
			Powers->Health ? Powers->Health->Health : -1.f, Powers->Health && Powers->Health->IsDead() ? 1 : 0,
			Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
	});
	// Fase 3 · Esquiva perfecta: una bala a 4 m viene hacia ti y haces dash → tiempo de reflejo, sin daño.
	Add(TEXT("esquiva_perfecta"), 2.f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		const FVector Forward = Character->GetActorForwardVector();
		SpawnTourBullet(Character->GetActorLocation() + Forward * 400.f + FVector(0.f, 0.f, 20.f), -Forward);
		const float Before = Powers->Health ? Powers->Health->Health : -1.f;
		const bool bDash = Powers->Dash && Powers->Dash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR esquiva dash=%d reflex=%d vida=%.0f"), bDash ? 1 : 0, Powers->Dash && Powers->Dash->IsReflexActive() ? 1 : 0, Before);
	});
	Add(TEXT("despues_del_reflejo"), 1.f, [this]
	{
		ClearTourTemps();
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR despues_del_reflejo vida=%.0f global=%.2f"), Powers->Health ? Powers->Health->Health : -1.f, Dilation ? Dilation->GetGlobalDilation() : -1.f);
	});
	// Fase 3 · Ojo lento: apuntando, dos balas pasan a tu lado y se frenan al entrar a 3.5 m.
	Add(TEXT("ojo_lento"), 1.f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		if (Powers->Aim)
		{
			Powers->Aim->SetAiming(true);
		}
		const FVector Forward = Character->GetActorForwardVector();
		const FVector Right = Character->GetActorRightVector();
		const FVector Chest = Character->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		SpawnTourBullet(Chest + Forward * 600.f + Right * 90.f, -Forward);
		SpawnTourBullet(Chest + Forward * 700.f - Right * 110.f, -Forward);
	});
	Add(TEXT("ojo_lento_revisa"), 1.2f, [this]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_lento frenadas=%d vida=%.0f"), Powers->SlowEye ? Powers->SlowEye->NumSlowed() : -1, Powers->Health ? Powers->Health->Health : -1.f);
		if (Powers->Aim)
		{
			Powers->Aim->SetAiming(false);
		}
	});
	Add(TEXT("ojo_lento_fin"), 0.5f, [this] { ClearTourTemps(); });
	// Fase 3 · Remates: por la espalda (maniquí 0, zona segura) y de frente pero congelado con Detener (maniquí 2).
	auto Mannequin = [this](int32 Index) -> ATemporalMannequin*
	{
		return Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().IsValidIndex(Index) ? Rooms->ReflexRoom->GetMannequins()[Index].Get() : nullptr;
	};
	auto StandAt = [this, Player](const FVector& Where, const FVector& LookAt)
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		Character->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
		const FRotator Facing(0.f, (LookAt - Where).Rotation().Yaw, 0.f);
		Character->SetActorRotation(Facing);
		if (AController* Controller = Character->GetController())
		{
			Controller->SetControlRotation(FRotator(-15.f, Facing.Yaw, 0.f));
		}
	};
	Add(TEXT("remate_acercarse"), 1.f, [this, Mannequin, StandAt]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(30.f); }
		if (ATemporalMannequin* Target = Mannequin(0))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 150.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("remate_espalda"), 1.f, [this]
	{
		const bool bShown = Powers->Takedown && Powers->Takedown->GetAvailableTarget();
		const bool bOk = Powers->Takedown && Powers->Takedown->TryTakedown();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_espalda aviso=%d ok=%d"), bShown ? 1 : 0, bOk ? 1 : 0);
	});
	Add(TEXT("maniqui_cae"), 2.f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_cae tirado=%d"), Mannequin(0) && Mannequin(0)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("remate_congelado"), 1.4f, [this, Mannequin, StandAt]
	{
		ATemporalMannequin* Target = Mannequin(2);
		if (!Target)
		{
			return;
		}
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		StandAt(Target->GetActorLocation() + Target->GetActorForwardVector() * 150.f, Target->GetActorLocation());
		const bool bBefore = Powers->Takedown && Powers->Takedown->TryTakedown();
		if (Powers->TimeStop) { Powers->TimeStop->StopActor(Target); }
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_congelado sin_congelar=%d"), bBefore ? 1 : 0);
	});
	Add(TEXT("remate_congelado_golpe"), 1.2f, [this, Mannequin]
	{
		const bool bOk = Powers->Takedown && Powers->Takedown->TryTakedown();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_congelado ok=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("remate_congelado_guardado"), 1.f, [this, Mannequin]
	{
		ATemporalMannequin* Target = Mannequin(2);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR golpe_guardado=%d tirado=%d"), Target && Target->HasPendingHit() ? 1 : 0, Target && Target->IsKnockedDown() ? 1 : 0);
		if (Powers->TimeStop && Target) { Powers->TimeStop->ReleaseActor(Target); }
	});
	Add(TEXT("remate_congelado_sale"), 1.2f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR al_soltar tirado=%d"), Mannequin(2) && Mannequin(2)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("maniqui_se_levanta"), 3.f, [] {});
	Add(TEXT("maniqui_de_pie"), 0.5f, [this, Mannequin]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_de_pie m0_tirado=%d m2_tirado=%d"),
			Mannequin(0) && Mannequin(0)->IsKnockedDown() ? 1 : 0, Mannequin(2) && Mannequin(2)->IsKnockedDown() ? 1 : 0);
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Revisión final · Detener con la mira de verdad: apuntando (Ojo lento) a una bala que viene hacia ti y clic izquierdo.
	Add(TEXT("detener_mira_prepara"), 0.5f, [this]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(30.f); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
	});
	// La bala viene justo por la línea de la mira (como cuando la sigues con el ratón); detrás quedan la torreta y
	// los maniquíes, que antes se robaban el clic.
	Add(TEXT("detener_mira_bala"), 0.25f, [this]
	{
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
			const FVector Forward = CameraRotation.Vector();
			SpawnTourBullet(CameraLocation + Forward * 700.f, -Forward);
		}
	});
	Add(TEXT("detener_mira_clic"), 1.f, [this]
	{
		if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0); PC && TourTemps.Num() > 0 && TourTemps.Last().IsValid())
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
			const FVector ToBullet = TourTemps.Last()->GetActorLocation() - CameraLocation;
			const APawn* Pawn = PC->GetPawn();
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR detener_mira_diag dist_jugador=%.0f angulo=%.1f ritmo=%.2f"),
				Pawn ? FVector::Dist(Pawn->GetActorLocation(), TourTemps.Last()->GetActorLocation()) : -1.f,
				FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ToBullet.GetSafeNormal(), CameraRotation.Vector()))),
				TourTemps.Last()->CustomTimeDilation);
		}
		// Fase 4: Detener se mudó a Q apuntando a algo (el clic apuntando dispara).
		Powers->SpawnBubble();
		AActor* Bullet = TourTemps.Num() > 0 ? TourTemps.Last().Get() : nullptr;
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const AActor* Hovered = Powers->Aim ? Powers->Aim->GetHoveredTarget() : nullptr;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR detener_con_mira detenida=%d congelada=%d seleccion=%s"),
			Bullet && Powers->TimeStop && Powers->TimeStop->IsStoppedByMe(Bullet) ? 1 : 0, Bullet && Dilation && Dilation->IsFrozen(Bullet) ? 1 : 0,
			Hovered ? *Hovered->GetClass()->GetName() : TEXT("nada"));
	});
	Add(TEXT("detener_mira_fin"), 0.3f, [this]
	{
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		ClearTourTemps();
	});
	// Revisión final · Dash hacia la espalda de un maniquí y rematar a medio dash (con doble clic).
	Add(TEXT("dash_remate_prepara"), 0.3f, [this, Mannequin, StandAt]
	{
		if (ATemporalMannequin* Target = Mannequin(0))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 260.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("dash_hacia_maniqui"), 0.1f, [this]
	{
		const bool bOk = Powers->Dash && Powers->Dash->TryDash();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dash_hacia_maniqui dash=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("remate_a_medio_dash"), 0.8f, [this]
	{
		Powers->PrimaryAction();
		const bool bBusy = Powers->Takedown && Powers->Takedown->IsBusy();
		const bool bStillDashing = Powers->Dash && Powers->Dash->IsDashing();
		// El doble clic se prueba apuntando (lo que antes detenía al maniquí a medio golpe).
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
		Powers->PrimaryAction();
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_en_dash remate=%d sigue_dash=%d detenidos_tras_doble_clic=%d"),
			bBusy ? 1 : 0, bStillDashing ? 1 : 0, Powers->TimeStop ? Powers->TimeStop->NumStopped() : -1);
	});
	Add(TEXT("remate_en_dash_fin"), 0.3f, [this]
	{
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 3b · Parkour en la sala 6: Espacio frente a cada obstáculo (por el mismo camino que la tecla: Jump()).
	auto ParkourAt = [this, Player](const FVector& Local, bool bRunning)
	{
		ACharacter* Character = Player();
		if (!Character || !Rooms->ParkourRoom)
		{
			return;
		}
		Rooms->ParkourRoom->PlacePlayer(Local);
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetSprint(bRunning);
		}
		if (bRunning)
		{
			Character->GetCharacterMovement()->Velocity = Character->GetActorForwardVector() * 700.f;
		}
		Character->Jump();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR parkour=%s"), Powers->Parkour ? EntropiaParkour::Name(Powers->Parkour->GetCurrent()) : TEXT("?"));
	};
	auto LogWhere = [this, Player](const TCHAR* Name)
	{
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetSprint(false);
		}
		const FVector Local = Player() && Rooms->ParkourRoom ? Rooms->ParkourRoom->GetActorTransform().InverseTransformPosition(Player()->GetActorLocation()) : FVector::ZeroVector;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR %s x=%.0f z=%.0f modo=%d"), Name, Local.X, Local.Z,
			Player() ? static_cast<int32>(Player()->GetCharacterMovement()->MovementMode) : -1);
	};
	Add(TEXT("sala6"), 1.5f, [this] { Rooms->GoToRoom(6); });
	Add(TEXT("salto_normal"), 0.4f, [this, Player]
	{
		if (Rooms->ParkourRoom)
		{
			Rooms->ParkourRoom->PlacePlayer(FVector(150.f, 0.f, 98.f));
		}
		if (Player())
		{
			Player()->Jump();
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR salto_normal parkour=%s"), Powers->Parkour ? EntropiaParkour::Name(Powers->Parkour->GetCurrent()) : TEXT("?"));
	});
	Add(TEXT("salto_normal_revisa"), 0.6f, [LogWhere] { LogWhere(TEXT("salto_normal_fin")); });
	Add(TEXT("dash_vault"), 0.7f, [ParkourAt] { ParkourAt(FVector(490.f, -450.f, 98.f), false); });
	Add(TEXT("dash_vault_fin"), 0.6f, [LogWhere] { LogWhere(TEXT("dash_vault_fin")); });
	Add(TEXT("gato"), 0.75f, [ParkourAt] { ParkourAt(FVector(1035.f, -450.f, 98.f), true); });
	Add(TEXT("gato_fin"), 0.6f, [LogWhere] { LogWhere(TEXT("gato_fin")); });
	Add(TEXT("subir"), 0.6f, [ParkourAt] { ParkourAt(FVector(1710.f, -450.f, 98.f), false); });
	Add(TEXT("subir_fin"), 0.6f, [LogWhere] { LogWhere(TEXT("subir_fin")); });
	Add(TEXT("trepar"), 1.1f, [ParkourAt] { ParkourAt(FVector(710.f, 150.f, 98.f), false); });
	Add(TEXT("trepar_fin"), 0.6f, [LogWhere] { LogWhere(TEXT("trepar_fin")); });
	Add(TEXT("pared"), 1.3f, [ParkourAt] { ParkourAt(FVector(1810.f, 150.f, 98.f), true); });
	Add(TEXT("pared_fin"), 0.8f, [LogWhere] { LogWhere(TEXT("pared_fin")); });
	// Fase 3b · Orillas: saltar hacia el muro de 3.4 m (fuera de alcance para trepar) y colgarse del borde.
	auto Hold = [this](const FVector& Direction)
	{
		if (Powers->Locomotion)
		{
			Powers->Locomotion->SetTourInput(Direction);
		}
	};
	auto LogHang = [this, Player](const TCHAR* Name)
	{
		const FVector Local = Player() && Rooms->ParkourRoom ? Rooms->ParkourRoom->GetActorTransform().InverseTransformPosition(Player()->GetActorLocation()) : FVector::ZeroVector;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR %s colgado=%d y=%.0f z=%.0f"), Name, Powers->Parkour && Powers->Parkour->IsHanging() ? 1 : 0, Local.Y, Local.Z);
	};
	Add(TEXT("colgarse"), 1.2f, [this, Player, Hold]
	{
		if (Rooms->ParkourRoom)
		{
			Rooms->ParkourRoom->PlacePlayer(FVector(600.f, 600.f, 98.f));
			Hold(Rooms->ParkourRoom->GetActorForwardVector());
		}
		if (Player())
		{
			Player()->Jump();
		}
	});
	Add(TEXT("orilla_de_lado"), 1.f, [this, Hold, LogHang]
	{
		LogHang(TEXT("colgarse"));
		if (Rooms->ParkourRoom)
		{
			Hold(Rooms->ParkourRoom->GetActorRightVector());
		}
	});
	Add(TEXT("orilla_sube"), 1.f, [this, Player, Hold, LogHang]
	{
		LogHang(TEXT("orilla_de_lado"));
		if (Rooms->ParkourRoom)
		{
			Hold(Rooms->ParkourRoom->GetActorForwardVector());
		}
		if (Player())
		{
			Player()->Jump();
		}
	});
	Add(TEXT("orilla_arriba"), 0.5f, [Hold, LogHang]
	{
		LogHang(TEXT("orilla_sube"));
		Hold(FVector::ZeroVector);
	});
	Add(TEXT("orilla_soltar"), 1.2f, [this, Player, Hold, LogHang]
	{
		if (Rooms->ParkourRoom)
		{
			Rooms->ParkourRoom->PlacePlayer(FVector(600.f, 650.f, 98.f));
			Hold(Rooms->ParkourRoom->GetActorForwardVector());
		}
		if (Player())
		{
			Player()->Jump();
		}
	});
	Add(TEXT("orilla_soltar_c"), 0.8f, [this, Hold, LogHang]
	{
		LogHang(TEXT("antes_de_soltar"));
		Hold(FVector::ZeroVector);
		if (Powers->Parkour)
		{
			Powers->Parkour->Drop();
		}
	});
	Add(TEXT("orilla_suelto"), 0.3f, [LogHang] { LogHang(TEXT("suelto")); });
	// Fase 3b · Escalera de bloques de 2 m (trepar 5 veces) hasta un balcón de 10 m; bajarse por el frente = caída alta.
	Add(TEXT("escalera"), 1.1f, [this, Player, Hold]
	{
		if (Rooms->ParkourRoom)
		{
			Rooms->ParkourRoom->PlacePlayer(FVector(1610.f, 600.f, 98.f));
			Hold(Rooms->ParkourRoom->GetActorForwardVector());
		}
		if (Player()) { Player()->Jump(); }
	});
	Add(TEXT("escalera_2"), 1.1f, [Player] { if (Player()) { Player()->Jump(); } });
	Add(TEXT("escalera_3"), 1.1f, [Player] { if (Player()) { Player()->Jump(); } });
	Add(TEXT("escalera_4"), 1.1f, [Player] { if (Player()) { Player()->Jump(); } });
	Add(TEXT("escalera_5"), 0.95f, [Player] { if (Player()) { Player()->Jump(); } });
	Add(TEXT("balcon"), 0.4f, [Hold, LogWhere]
	{
		Hold(FVector::ZeroVector);
		LogWhere(TEXT("balcon"));
	});
	Add(TEXT("balcon_bajarse"), 2.2f, [this, Hold]
	{
		if (Powers->Locomotion) { Powers->Locomotion->ResetFall(); }
		if (Rooms->ParkourRoom) { Hold(Rooms->ParkourRoom->GetActorForwardVector()); }
	});
	Add(TEXT("balcon_revisa"), 0.3f, [this, Hold, LogWhere]
	{
		const FEntropiaLandingInfo Info = Powers->Locomotion ? Powers->Locomotion->GetLastLanding() : FEntropiaLandingInfo();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR balcon_caida tipo=%d altura=%.0f dano=%.0f"), static_cast<int32>(Info.Kind), Info.Height, Info.Damage);
		Hold(FVector::ZeroVector);
		LogWhere(TEXT("balcon_abajo"));
		if (Powers->Health) { Powers->Health->ResetHealth(); }
	});
	// Fase 3b · Cobertura: barda baja (agachado, asomarse levantándose) y muro alto (esquina, asomarse de lado).
	auto LogCover = [this, Player](const TCHAR* Name)
	{
		const FVector Local = Player() && Rooms->ParkourRoom ? Rooms->ParkourRoom->GetActorTransform().InverseTransformPosition(Player()->GetActorLocation()) : FVector::ZeroVector;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR %s cubierto=%d tipo=%d esquina=%d agachado=%d asomado=%d x=%.0f y=%.0f"), Name,
			Powers->Cover && Powers->Cover->IsInCover() ? 1 : 0, Powers->Cover ? static_cast<int32>(Powers->Cover->GetKind()) : -1,
			Powers->Cover ? Powers->Cover->GetCornerSide() : 0, Powers->Locomotion && Powers->Locomotion->IsCrouched() ? 1 : 0,
			Powers->Cover && Powers->Cover->IsPeeking() ? 1 : 0, Local.X, Local.Y);
	};
	// Fase 3b · La pose de cobertura baja sale de su hueco en DA_Animaciones (se cambia por otra un momento y se regresa).
	TSharedRef<FEntropiaAnimSlot> SavedCoverLow = MakeShared<FEntropiaAnimSlot>();
	Add(TEXT("cobertura_pose"), 0.5f, [this, SavedCoverLow]
	{
		if (UEntropiaAnimationSet* Set = const_cast<UEntropiaAnimationSet*>(UEntropiaAnimationSet::Get()))
		{
			*SavedCoverLow = Set->CoverLow;
			Set->CoverLow.Anim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump")));
		}
		if (Rooms->ParkourRoom) { Rooms->ParkourRoom->PlacePlayer(FVector(490.f, -450.f, 98.f)); }
		if (Powers->Cover) { Powers->Cover->TryEnter(); }
	});
	Add(TEXT("cobertura_pose_revisa"), 0.3f, [this, Player, SavedCoverLow]
	{
		if (UEntropiaAnimationSet* Set = const_cast<UEntropiaAnimationSet*>(UEntropiaAnimationSet::Get()))
		{
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR cobertura_pose propia=%d"), Player() && EntropiaAnim::IsPlaying(Player(), Set->CoverLow) ? 1 : 0);
			Set->CoverLow = *SavedCoverLow;
		}
		if (Powers->Cover) { Powers->Cover->Exit(); }
	});
	Add(TEXT("cubrirse_bajo"), 0.8f, [this, LogCover]
	{
		if (Rooms->ParkourRoom) { Rooms->ParkourRoom->PlacePlayer(FVector(490.f, -450.f, 98.f)); }
		if (Powers->Cover) { Powers->Cover->TryEnter(); }
	});
	Add(TEXT("cubierto_moverse"), 0.8f, [this, Hold, LogCover]
	{
		LogCover(TEXT("cubrirse_bajo"));
		if (Rooms->ParkourRoom) { Hold(-Rooms->ParkourRoom->GetActorRightVector()); }
	});
	Add(TEXT("asomarse_bajo"), 0.8f, [this, Hold, LogCover]
	{
		Hold(FVector::ZeroVector);
		LogCover(TEXT("cubierto_moverse"));
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
	});
	Add(TEXT("cubrirse_alto"), 0.8f, [this, LogCover]
	{
		LogCover(TEXT("asomarse_bajo"));
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		if (Powers->Cover) { Powers->Cover->Exit(); }
		if (Rooms->ParkourRoom) { Rooms->ParkourRoom->PlacePlayer(FVector(640.f, 480.f, 98.f)); }
		if (Powers->Cover) { Powers->Cover->TryEnter(); }
	});
	Add(TEXT("asomarse_esquina"), 0.8f, [this, LogCover]
	{
		LogCover(TEXT("cubrirse_alto"));
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
	});
	Add(TEXT("salir_cobertura"), 0.5f, [this, LogCover]
	{
		LogCover(TEXT("asomarse_esquina"));
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		if (Powers->Cover) { Powers->Cover->Exit(); }
	});
	Add(TEXT("salir_revisa"), 0.3f, [LogCover] { LogCover(TEXT("salir_cobertura")); });
	// Fase 3b · Sala 7: dummies armados que se asoman y disparan (protegido por la barda baja).
	auto DummyShots = [this]()
	{
		int32 Total = 0;
		if (Rooms->CombatRoom)
		{
			for (const ATemporalArmedDummy* Dummy : Rooms->CombatRoom->GetDummies())
			{
				Total += Dummy ? Dummy->GetTotalShots() : 0;
			}
		}
		return Total;
	};
	Add(TEXT("sala7"), 1.5f, [this] { Rooms->GoToRoom(7); if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); } });
	Add(TEXT("dummy_dispara"), 4.5f, [this, DummyShots]
	{
		if (Rooms->CombatRoom) { Rooms->CombatRoom->PlacePlayer(FVector(900.f, 0.f, 98.f)); }
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR sala7 disparos_antes=%d"), DummyShots());
	});
	Add(TEXT("dummy_dispara_revisa"), 0.3f, [this, DummyShots]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dummies_dispararon=%d"), DummyShots() > 0 ? 1 : 0);
	});
	// Fase 3b · Al disparar, el dummy usa su hueco DummyFire de DA_Animaciones.
	Add(TEXT("dummy_dispara_pose"), 0.3f, [this, Player]
	{
		ATemporalArmedDummy* Target = Rooms->CombatRoom && Rooms->CombatRoom->GetDummies().IsValidIndex(0) ? Rooms->CombatRoom->GetDummies()[0].Get() : nullptr;
		const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
		if (Target && Player())
		{
			// Hacia el centro de la sala (lado libre, sin la pared del dummy en medio).
			Player()->SetActorLocation(Target->GetActorLocation() + Rooms->CombatRoom->GetActorRightVector() * 250.f, false, nullptr, ETeleportType::TeleportPhysics);
			const int32 Before = Target->GetTotalShots();
			Target->FireAtPlayer();
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR dummy_dispara_pose disparo=%d pose=%d"), Target->GetTotalShots() - Before,
				Set && EntropiaAnim::IsPlaying(Target, Set->DummyFire) ? 1 : 0);
		}
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 3b · Golpes: combo de 3 al maniquí 0 de la sala 5 (el tercero lo tumba); romper el cuello por la espalda;
	// golpe con enfoque a uno congelado de frente.
	auto M = [this](int32 Index) -> ATemporalMannequin*
	{
		return Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().IsValidIndex(Index) ? Rooms->ReflexRoom->GetMannequins()[Index].Get() : nullptr;
	};
	Add(TEXT("golpes_prepara"), 0.6f, [this, M, StandAt]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		if (ATemporalMannequin* Target = M(0))
		{
			StandAt(Target->GetActorLocation() + Target->GetActorForwardVector() * 110.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("golpe_1"), 0.35f, [this] { if (Powers->Melee) { Powers->Melee->TryAttack(); } });
	Add(TEXT("golpe_2"), 0.35f, [this] { if (Powers->Melee) { Powers->Melee->TryAttack(); } });
	Add(TEXT("golpe_3"), 0.8f, [this] { if (Powers->Melee) { Powers->Melee->TryAttack(); } });
	Add(TEXT("golpes_revisa"), 0.3f, [this, M]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR golpes ultimo=%d conectaron=%d tirado=%d"),
			Powers->Melee ? Powers->Melee->GetLastIndex() : -1, Powers->Melee ? Powers->Melee->GetHitsLanded() : -1,
			M(0) && M(0)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("romper_cuello"), 1.2f, [this, M, StandAt]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		if (ATemporalMannequin* Target = M(0))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 120.f, Target->GetActorLocation());
		}
	});
	Add(TEXT("romper_cuello_clic"), 0.9f, [this]
	{
		Powers->PrimaryAction();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR romper_cuello tipo=%d"), Powers->Takedown ? static_cast<int32>(Powers->Takedown->GetLastKind()) : -1);
	});
	Add(TEXT("romper_cuello_revisa"), 0.6f, [this, M]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR cuello_roto tirado=%d"), M(0) && M(0)->IsKnockedDown() ? 1 : 0);
	});
	Add(TEXT("golpe_enfoque"), 0.8f, [this, M, StandAt]
	{
		if (ATemporalMannequin* Target = M(2))
		{
			StandAt(Target->GetActorLocation() + Target->GetActorForwardVector() * 140.f, Target->GetActorLocation());
			if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
			if (Powers->TimeStop) { Powers->TimeStop->StopActor(Target); }
		}
	});
	Add(TEXT("golpe_enfoque_clic"), 0.5f, [this]
	{
		Powers->PrimaryAction();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR golpe_enfoque tipo=%d"), Powers->Takedown ? static_cast<int32>(Powers->Takedown->GetLastKind()) : -1);
	});
	Add(TEXT("golpe_enfoque_suelta"), 1.f, [this, M]
	{
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 3b · Agarrar al dummy armado A de la sala 7 por detrás: lo desarmas y lo sometes; luego le rompes el cuello.
	// Después agarras al B y lo sueltas con un empujón (E otra vez).
	auto Dummy = [this](int32 Index) -> ATemporalArmedDummy*
	{
		return Rooms->CombatRoom && Rooms->CombatRoom->GetDummies().IsValidIndex(Index) ? Rooms->CombatRoom->GetDummies()[Index].Get() : nullptr;
	};
	auto BehindDummy = [this, Dummy, StandAt](int32 Index)
	{
		if (ATemporalArmedDummy* Target = Dummy(Index))
		{
			StandAt(Target->GetActorLocation() - Target->GetActorForwardVector() * 110.f, Target->GetActorLocation());
		}
	};
	Add(TEXT("agarrar_prepara"), 0.5f, [this, BehindDummy]
	{
		Rooms->GoToRoom(7);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		BehindDummy(0);
	});
	Add(TEXT("agarrar_desarmar"), 0.8f, [this, Dummy]
	{
		const bool bOk = Powers->Grab && Powers->Grab->TryGrab();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR agarrar ok=%d armado=%d sometido=%d"), bOk ? 1 : 0,
			Dummy(0) && Dummy(0)->IsArmed() ? 1 : 0, Dummy(0) && Dummy(0)->IsSubdued() ? 1 : 0);
	});
	Add(TEXT("sometido_cuello"), 1.4f, [this]
	{
		Powers->PrimaryAction();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR sometido_cuello tipo=%d"), Powers->Takedown ? static_cast<int32>(Powers->Takedown->GetLastKind()) : -1);
	});
	Add(TEXT("sometido_revisa"), 0.3f, [this, Dummy]
	{
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR sometido_fin tirado=%d agarrando=%d modo=%d"), Dummy(0) && Dummy(0)->IsKnockedDown() ? 1 : 0,
			Powers->Grab && Powers->Grab->IsHolding() ? 1 : 0, Character ? static_cast<int32>(Character->GetCharacterMovement()->MovementMode) : -1);
	});
	Add(TEXT("agarrar_soltar"), 0.6f, [this, BehindDummy]
	{
		BehindDummy(1);
		if (Powers->Grab) { Powers->Grab->TryGrab(); }
	});
	Add(TEXT("soltar_empujon"), 0.4f, [this]
	{
		if (Powers->Grab && Powers->Grab->IsHolding()) { Powers->Grab->Release(true); }
	});
	Add(TEXT("soltar_revisa"), 0.3f, [this, Dummy]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR soltar sometido=%d aturdido=%d desarmado=%d"), Dummy(1) && Dummy(1)->IsSubdued() ? 1 : 0,
			Dummy(1) && Dummy(1)->IsStaggered() ? 1 : 0, Dummy(1) && !Dummy(1)->IsArmed() ? 1 : 0);
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 3b · Detener nivel 2 (The World): todo se detiene menos tú; el remate a un congelado sale al volver el tiempo.
	Add(TEXT("mundo_prepara"), 0.8f, [this, M, StandAt]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		if (ATemporalMannequin* Target = M(2))
		{
			StandAt(Target->GetActorLocation() + Target->GetActorForwardVector() * 140.f, Target->GetActorLocation());
		}
		if (Powers->GetTimeStopLevel() < 2) { Powers->CycleTimeStopLevel(); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
	});
	Add(TEXT("mundo_impacto"), 0.2f, [this]
	{
		Powers->SpawnBubble();
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR mundo_detenido activo=%d global=%.4f"), Powers->WorldStop && Powers->WorldStop->IsActive() ? 1 : 0,
			Dilation ? Dilation->GetGlobalDilation() : -1.f);
	});
	Add(TEXT("mundo_congelado"), 1.5f, [this]
	{
		Powers->PrimaryAction();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR mundo_remate tipo=%d"), Powers->Takedown ? static_cast<int32>(Powers->Takedown->GetLastKind()) : -1);
	});
	Add(TEXT("mundo_regresa"), 2.6f, [this, M]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR mundo_durante golpe_guardado=%d"), M(2) && M(2)->HasPendingHit() ? 1 : 0);
	});
	Add(TEXT("mundo_fin"), 0.8f, [this, M]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR mundo_fin activo=%d global=%.2f tirado=%d"), Powers->WorldStop && Powers->WorldStop->IsActive() ? 1 : 0,
			Dilation ? Dilation->GetGlobalDilation() : -1.f, M(2) && M(2)->IsKnockedDown() ? 1 : 0);
		Powers->CycleTimeStopLevel();
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 4 · Las balas del jugador dañan: cabeza tumba, torso aturde, a un congelado se le guardan; Devuélvesela.
	auto ShotDummy = [this](int32 Index) -> ATemporalArmedDummy*
	{
		return Rooms->CombatRoom && Rooms->CombatRoom->GetDummies().IsValidIndex(Index) ? Rooms->CombatRoom->GetDummies()[Index].Get() : nullptr;
	};
	// Dispara una bala del jugador desde atrás del dummy (ahí no hay muro): a la cabeza o al torso.
	auto ShootDummy = [this, ShotDummy](int32 Index, bool bHead, float Torso, float Head, float Impulse)
	{
		if (ATemporalArmedDummy* Target = ShotDummy(Index))
		{
			const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
			const float Z = bHead ? Capsule->GetScaledCapsuleHalfHeight() - 12.f : 0.f;
			const FVector Aim = Capsule->GetComponentLocation() + FVector(0.f, 0.f, Z);
			// 150 cm detrás: el dummy del fondo está a 250 cm de la pared.
			const FVector From = Aim - Target->GetActorForwardVector() * 150.f;
			SpawnPlayerTourBullet(From, (Aim - From).GetSafeNormal(), Torso, Head, Impulse);
		}
	};
	auto StuckBullets = [this](const AActor* Target)
	{
		int32 Count = 0;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			Count += Target && It->GetAttachParentActor() == Target ? 1 : 0;
		}
		return Count;
	};
	Add(TEXT("armas_prepara"), 1.2f, [this]
	{
		Rooms->GoToRoom(7);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
	});
	Add(TEXT("tiro_cabeza"), 0.5f, [ShootDummy] { ShootDummy(0, true, 55.f, 200.f, 900.f); });
	Add(TEXT("tiro_torso"), 0.5f, [this, ShotDummy, ShootDummy]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tiro_cabeza tumbado=%d"), ShotDummy(0) && ShotDummy(0)->IsKnockedDown() ? 1 : 0);
		ShootDummy(1, false, 55.f, 200.f, 900.f);
	});
	Add(TEXT("tiro_congelado"), 0.5f, [this, ShotDummy, ShootDummy]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tiro_torso tumbado=%d aguante=%.0f"), ShotDummy(1) && ShotDummy(1)->IsKnockedDown() ? 1 : 0,
			ShotDummy(1) ? ShotDummy(1)->GetToughness() : -1.f);
		if (Powers->TimeStop && ShotDummy(2)) { Powers->TimeStop->StopActor(ShotDummy(2)); }
		for (int32 i = 0; i < 3; ++i)
		{
			ShootDummy(2, false, 20.f, 60.f, 450.f);
		}
	});
	Add(TEXT("tiro_congelado_suelta"), 0.6f, [this, ShotDummy, StuckBullets]
	{
		const ATemporalArmedDummy* Target = ShotDummy(2);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tiro_congelado guardados=%d pegadas=%d tumbado=%d aguante=%.0f"), Target ? Target->NumStoredShots() : -1,
			StuckBullets(Target), Target && Target->IsKnockedDown() ? 1 : 0, Target ? Target->GetToughness() : -1.f);
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
	});
	Add(TEXT("devuelvesela_prepara"), 0.4f, [this, ShotDummy, StuckBullets]
	{
		const ATemporalArmedDummy* Target = ShotDummy(2);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tiro_congelado_fin guardados=%d pegadas=%d aguante=%.0f"), Target ? Target->NumStoredShots() : -1,
			StuckBullets(Target), Target ? Target->GetToughness() : -1.f);
		if (Rooms->CombatRoom) { Rooms->CombatRoom->ResetRoom(); }
	});
	Add(TEXT("devuelvesela_dispara"), 0.15f, [this, ShotDummy]
	{
		ATemporalArmedDummy* Target = ShotDummy(1);
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Target && Pawn && Rooms->CombatRoom)
		{
			// Frente a él (sin muro en medio): así sí te ve y dispara.
			Pawn->SetActorLocation(Target->GetActorLocation() + Rooms->CombatRoom->GetActorRightVector() * -250.f + FVector(0.f, 0.f, 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			Target->FireAtPlayer();
		}
	});
	Add(TEXT("devuelvesela"), 1.2f, [this, ShotDummy]
	{
		ATemporalArmedDummy* Target = ShotDummy(1);
		ATemporalProjectile* Theirs = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			if (It->GetOwner() == Target)
			{
				Theirs = *It;
			}
		}
		const bool bStarted = Theirs && Powers->Inversion && Powers->Inversion->BeginInversionOn(Theirs->Recorder);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR devuelvesela bala=%d"), Theirs ? 1 : 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR diag_devuelvesela invierte=%d grabado=%.2f"), bStarted ? 1 : 0, Theirs ? Theirs->Recorder->GetRecordedSeconds() : -1.f);
	});
	Add(TEXT("devuelvesela_revisa"), 0.3f, [this, ShotDummy]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR devuelvesela_fin dummy_tumbado=%d"), ShotDummy(1) && ShotDummy(1)->IsKnockedDown() ? 1 : 0);
		if (Rooms->CombatRoom) { Rooms->CombatRoom->ResetRoom(); }
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 4 · El arma del jugador: disparar, recargar, cambiar, ráfaga, Recarga inversa, Último en caer y Q = Detener.
	auto WeaponDummy = [this](int32 Index) -> ATemporalArmedDummy*
	{
		return Rooms->CombatRoom && Rooms->CombatRoom->GetDummies().IsValidIndex(Index) ? Rooms->CombatRoom->GetDummies()[Index].Get() : nullptr;
	};
	// De lado al dummy (sin muro en medio), apuntando.
	auto BesideDummy = [this, WeaponDummy](int32 Index)
	{
		ATemporalArmedDummy* Target = WeaponDummy(Index);
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Target && Pawn && Rooms->CombatRoom)
		{
			const FVector Side = Rooms->CombatRoom->GetActorRightVector() * (Target->GetActorLocation().Y > Rooms->CombatRoom->GetActorLocation().Y ? -1.f : 1.f);
			Pawn->SetActorLocation(Target->GetActorLocation() + Side * 600.f + FVector(0.f, 0.f, 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
	};
	auto HeadOf = [WeaponDummy](int32 Index)
	{
		const ATemporalArmedDummy* Target = WeaponDummy(Index);
		return Target ? Target->GetCapsuleComponent()->GetComponentLocation() + FVector(0.f, 0.f, Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 14.f) : FVector::ZeroVector;
	};
	auto LogWeapon = [this](const TCHAR* Tag)
	{
		const UEntropiaWeaponComponent* W = Powers->Weapons;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR %s arma=%s balas=%d recargando=%d vivas=%d tiros=%d"), Tag, W ? *W->CurrentStats().Name.ToString() : TEXT("nada"),
			W ? W->CurrentState().Ammo : -1, W && W->CurrentState().IsReloading() ? 1 : 0, W ? W->NumLiveBullets() : -1, W ? W->GetTotalShots() : -1);
	};
	// Vitrina (diagnóstico): el personaje con la pose de apuntar, visto de frente y de lado, para revisar el arma en la mano.
	auto Showcase = [this](int32 Weapon, float YawOffset)
	{
		if (Powers->Weapons && Powers->Weapons->GetCurrentIndex() != Weapon) { Powers->Weapons->CycleWeapon(1); }
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get();
		if (Character && PC && Set)
		{
			EntropiaAnim::Play(Character, Weapon == 1 ? Set->RifleAim : Set->RevolverAim);
			PC->SetControlRotation(FRotator(-12.f, Character->GetActorRotation().Yaw + YawOffset, 0.f));
		}
	};
	Add(TEXT("vitrina_prepara"), 0.6f, [this]
	{
		Rooms->GoToRoom(7);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
	});
	Add(TEXT("vitrina_revolver"), 0.8f, [Showcase] { Showcase(0, 140.f); });
	Add(TEXT("vitrina_revolver_lado"), 0.8f, [this, Showcase]
	{
		Showcase(0, 95.f);
		// Diagnóstico: ejes del hueso hand_r en la pose de apuntar (respecto a hacia dónde mira el personaje).
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (Character)
		{
			const USkeletalMeshComponent* Body = Character->GetMesh();
			const FTransform Hand = Body->GetSocketTransform(TEXT("hand_r"));
			const FVector F = Character->GetActorForwardVector(), R = Character->GetActorRightVector(), U = FVector::UpVector;
			auto Axis = [&](EAxis::Type A) { const FVector V = Hand.GetUnitAxis(A); return FString::Printf(TEXT("(f%.2f r%.2f u%.2f)"), FVector::DotProduct(V, F), FVector::DotProduct(V, R), FVector::DotProduct(V, U)); };
			const FVector Arm = (Hand.GetLocation() - Body->GetSocketLocation(TEXT("lowerarm_r"))).GetSafeNormal();
			const FVector Fingers = (Body->GetSocketLocation(TEXT("middle_01_r")) - Hand.GetLocation()).GetSafeNormal();
			UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR diag_mano X=%s Y=%s Z=%s brazo=(f%.2f r%.2f u%.2f) dedos=(f%.2f r%.2f u%.2f) weapon_r=%d"),
				*Axis(EAxis::X), *Axis(EAxis::Y), *Axis(EAxis::Z), FVector::DotProduct(Arm, F), FVector::DotProduct(Arm, R), FVector::DotProduct(Arm, U),
				FVector::DotProduct(Fingers, F), FVector::DotProduct(Fingers, R), FVector::DotProduct(Fingers, U), Body->DoesSocketExist(TEXT("weapon_r")) ? 1 : 0);
		}
	});
	Add(TEXT("vitrina_fusil"), 0.8f, [Showcase] { Showcase(1, 140.f); });
	Add(TEXT("vitrina_fusil_lado"), 0.8f, [Showcase] { Showcase(1, 95.f); });
	Add(TEXT("vitrina_fin"), 0.2f, [this]
	{
		if (Powers->Weapons && Powers->Weapons->GetCurrentIndex() != 0) { Powers->Weapons->CycleWeapon(1); }
		if (ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { EntropiaAnim::Stop(Character, 0.1f); }
	});
	Add(TEXT("revolver_prepara"), 0.8f, [this, BesideDummy]
	{
		Rooms->GoToRoom(7);
		if (Powers->Health) { Powers->Health->SetInvulnerable(60.f); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		BesideDummy(0);
	});
	Add(TEXT("revolver_apunta"), 0.5f, [this, HeadOf] { LookAtPoint(HeadOf(0)); });
	// La cámara está al hombro: al girar se mueve, así que se apunta dos veces (la segunda corrige la primera).
	Add(TEXT("revolver_apunta2"), 0.3f, [this, HeadOf] { LookAtPoint(HeadOf(0)); });
	Add(TEXT("revolver_disparo"), 0.6f, [this]
	{
		Powers->PrimaryAction();
		Powers->PrimaryReleased();
	});
	Add(TEXT("recargar"), 0.3f, [this, LogWeapon, WeaponDummy]
	{
		LogWeapon(TEXT("revolver_disparo"));
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR revolver_impacto tumbado=%d"), WeaponDummy(0) && WeaponDummy(0)->IsKnockedDown() ? 1 : 0);
		// R mirando al cielo (nada en la mira) = recargar.
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->SetControlRotation(FRotator(75.f, PC->GetControlRotation().Yaw, 0.f)); }
	});
	Add(TEXT("recargar_r"), 1.8f, [this, LogWeapon]
	{
		const bool bReloaded = Powers->ReloadIfNothingAimed();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR recargar_r sin_objetivo=%d"), bReloaded ? 1 : 0);
		LogWeapon(TEXT("recargar"));
	});
	Add(TEXT("cambiar_arma"), 0.4f, [this, LogWeapon]
	{
		LogWeapon(TEXT("recargar_fin"));
		if (Powers->Weapons) { Powers->Weapons->CycleWeapon(1); }
		LogWeapon(TEXT("cambiar_arma"));
	});
	Add(TEXT("fusil_rafaga"), 3.2f, [this]
	{
		// Contra la pared del fondo de la sala.
		if (Rooms->CombatRoom) { LookAtPoint(Rooms->CombatRoom->RoomToWorld(FVector(2600.f, 0.f, 150.f))); }
		Powers->PrimaryAction();
	});
	Add(TEXT("fusil_recarga"), 2.1f, [this, LogWeapon]
	{
		Powers->PrimaryReleased();
		LogWeapon(TEXT("fusil_rafaga"));
		if (Powers->Weapons) { Powers->Weapons->Reload(); }
	});
	Add(TEXT("fusil_rafaga2"), 1.6f, [this]
	{
		if (Rooms->CombatRoom) { LookAtPoint(Rooms->CombatRoom->RoomToWorld(FVector(2600.f, 0.f, 150.f))); }
		Powers->PrimaryAction();
	});
	Add(TEXT("recarga_inversa"), 1.5f, [this, LogWeapon]
	{
		Powers->PrimaryReleased();
		LogWeapon(TEXT("fusil_rafaga2"));
		const UEntropiaWeaponComponent* W = Powers->Weapons;
		// La más nueva que ya lleva un rato en la pared (una recién disparada todavía no tiene nada grabado).
		ATemporalProjectile* Newest = nullptr;
		for (int32 i = W ? W->GetLiveBullets().Num() - 1 : -1; i >= 0 && !Newest; --i)
		{
			ATemporalProjectile* Candidate = W->GetLiveBullets()[i].Get();
			Newest = Candidate && Candidate->Recorder->GetRecordedSeconds() > 0.3f ? Candidate : nullptr;
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR recarga_inversa_antes balas=%d"), W ? W->CurrentState().Ammo : -1);
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		const bool bStarted = Newest && Powers->Inversion && Powers->Inversion->BeginInversionOn(Newest->Recorder);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR diag_recarga_inversa invierte=%d grabado=%.2f"), bStarted ? 1 : 0, Newest ? Newest->Recorder->GetRecordedSeconds() : -1.f);
	});
	Add(TEXT("ultimo_prepara"), 0.8f, [this, LogWeapon, WeaponDummy, BesideDummy]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		LogWeapon(TEXT("recarga_inversa"));
		if (Powers->Weapons) { Powers->Weapons->CycleWeapon(-1); }
		if (Rooms->CombatRoom) { Rooms->CombatRoom->ResetRoom(); }
		for (int32 i = 0; i < 2; ++i)
		{
			if (ATemporalArmedDummy* Down = WeaponDummy(i)) { Down->ReceiveTakedown(Down->GetActorForwardVector() * -1.f, 300.f); }
		}
		BesideDummy(2);
	});
	Add(TEXT("ultimo_apunta"), 0.4f, [this, HeadOf] { LookAtPoint(HeadOf(2)); });
	Add(TEXT("ultimo_apunta2"), 0.3f, [this, HeadOf] { LookAtPoint(HeadOf(2)); });
	Add(TEXT("ultimo_en_caer"), 0.3f, [this]
	{
		Powers->PrimaryAction();
		Powers->PrimaryReleased();
	});
	Add(TEXT("ultimo_revisa"), 0.9f, [this, WeaponDummy]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ultimo_en_caer tumbado=%d camara_lenta=%d global=%.2f"), WeaponDummy(2) && WeaponDummy(2)->IsKnockedDown() ? 1 : 0,
			Powers->Weapons && Powers->Weapons->IsKillCamActive() ? 1 : 0, Dilation ? Dilation->GetGlobalDilation() : -1.f);
	});
	Add(TEXT("q_detener_prepara"), 0.6f, [this, BesideDummy]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ultimo_fin camara_lenta=%d global=%.2f"), Powers->Weapons && Powers->Weapons->IsKillCamActive() ? 1 : 0,
			Dilation ? Dilation->GetGlobalDilation() : -1.f);
		if (Rooms->CombatRoom) { Rooms->CombatRoom->ResetRoom(); }
		// Sin balas en la pared: la mira las elegiría primero.
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It) { It->Destroy(); }
		BesideDummy(0);
	});
	Add(TEXT("q_detener"), 0.5f, [this, WeaponDummy]
	{
		if (const ATemporalArmedDummy* Target = WeaponDummy(0)) { LookAtPoint(Target->GetActorLocation()); }
	});
	Add(TEXT("q_detener_revisa"), 0.4f, [this, WeaponDummy]
	{
		Powers->SpawnBubble();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR q_detener detenido=%d burbujas=%d"), Powers->TimeStop && WeaponDummy(0) && Powers->TimeStop->IsStoppedByMe(WeaponDummy(0)) ? 1 : 0,
			Powers->TimeStop ? Powers->TimeStop->NumStopped() : -1);
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		if (Rooms->CombatRoom) { Rooms->CombatRoom->ResetRoom(); }
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	Add(TEXT("sala2"), 1.6f, [this, Player]
	{
		Rooms->GoToRoom(2);
		// Mirar hacia abajo, al vacío: así la captura muestra las plataformas.
		if (ACharacter* Character = Player())
		{
			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(FRotator(-28.f, Controller->GetControlRotation().Yaw, 0.f));
			}
		}
	});
	Add(TEXT("plataforma_cae"), 2.2f, [this]
	{
		if (Rooms->Chamber && Rooms->Chamber->GetPlatforms().Num() > 0 && IsValid(Rooms->Chamber->GetPlatforms()[0]))
		{
			Rooms->Chamber->GetPlatforms()[0]->Crumble();
		}
	});
	Add(TEXT("invertir_plataforma"), 3.f, [this]
	{
		if (Rooms->Chamber && Rooms->Chamber->GetPlatforms().Num() > 0 && IsValid(Rooms->Chamber->GetPlatforms()[0]) && Powers->Inversion)
		{
			Powers->Inversion->BeginInversionOn(Rooms->Chamber->GetPlatforms()[0]->Recorder);
		}
	});
	Add(TEXT("sala1_correr"), 1.4f, [this, Player]
	{
		if (Powers->Inversion)
		{
			Powers->Inversion->EndInversion();
		}
		Rooms->GoToRoom(1);
		if (ACharacter* Character = Player())
		{
			Character->LaunchCharacter(FVector(700.f, 250.f, 450.f), true, true);
		}
	});
	Add(TEXT("paso_inverso"), 1.8f, [this] { if (Powers->InverseStep) { Powers->InverseStep->BeginInverseStep(); } });
	Add(TEXT("eco"), 2.2f, [this]
	{
		if (Powers->InverseStep)
		{
			Powers->InverseStep->EndInverseStep();
		}
		Powers->CreateEcho();
	});

}
