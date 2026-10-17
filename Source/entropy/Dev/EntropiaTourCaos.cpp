// ENTROPÍA · Tour "caos": hacer cosas raras a propósito para buscar errores, fugas y estados rotos.

#include "EntropiaTourComponent.h"
#include "EntropiaParkourComponent.h"
#include "EntropiaCoverComponent.h"
#include "EntropiaGrabComponent.h"
#include "TemporalCombatRoom.h"
#include "TemporalArmedDummy.h"
#include "TemporalPhysicsLab.h"
#include "TemporalBreakable.h"
#include "EntropiaLocomotionComponent.h"
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

void UEntropiaTourComponent::BuildChaosSteps()
{
	auto Add = [this](const TCHAR* Name, float Seconds, TFunction<void()> Action)
	{
		TourSteps.Add({ Name, MoveTemp(Action), Seconds });
	};
	auto Player = [this]() { return Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); };
	auto BreakAll = [this]()
	{
		for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
		{
			It->Break(It->GetActorLocation() + FMath::VRand() * 30.f, FMath::VRand());
		}
	};
	auto LookAt = [this, Player](float Pitch)
	{
		if (ACharacter* Character = Player())
		{
			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(FRotator(Pitch, Controller->GetControlRotation().Yaw, 0.f));
			}
		}
	};

	Add(TEXT("sala4"), 1.5f, [this] { Rooms->GoToRoom(4); });
	Add(TEXT("spam_z_x10"), 1.5f, [this] { for (int32 i = 0; i < 10; ++i) { Rooms->Fire(); } });
	Add(TEXT("spam_x_x10"), 1.5f, [this] { for (int32 i = 0; i < 10; ++i) { Rooms->FireInverted(); } });
	Add(TEXT("romper_todo_x3"), 1.5f, [BreakAll] { BreakAll(); BreakAll(); BreakAll(); });
	Add(TEXT("spam_b_x10"), 1.f, [this] { for (int32 i = 0; i < 10; ++i) { Rooms->BreakInFront(); } });
	// Girar la cámara y presionar R en frames distintos (como al jugar): la cámara se actualiza al frame siguiente.
	Add(TEXT("mirar_cielo"), 0.6f, [LookAt] { LookAt(70.f); });
	Add(TEXT("r_al_cielo"), 1.f, [this] { if (Powers->Inversion) { Powers->Inversion->BeginInversion(); } });
	Add(TEXT("r_suelta_y_mira"), 0.6f, [this, LookAt]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		LookAt(-15.f);
	});
	// Fase 3: apuntar y soltar muchas veces, e invertir mientras apuntas (la cámara y el giro deben quedar bien).
	Add(TEXT("apuntar_x11_y_r"), 1.f, [this]
	{
		for (int32 i = 0; i < 11; ++i) { if (Powers->Aim) { Powers->Aim->SetAiming(!Powers->Aim->IsAiming()); } }
		if (Powers->Inversion) { Powers->Inversion->BeginInversion(); }
	});
	Add(TEXT("suelta_apuntar"), 0.6f, [this]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
	});
	Add(TEXT("shiftr_y_romper"), 2.f, [this, BreakAll] { Rooms->RewindAll(true); BreakAll(); Rooms->Fire(); });
	Add(TEXT("shiftr_suelta"), 0.8f, [this] { Rooms->RewindAll(false); });
	Add(TEXT("r_y_teletransporte"), 1.2f, [this]
	{
		Rooms->RewindAll(true);
		Rooms->GoToRoom(4);
		Rooms->GoToRoom(4);
		Rooms->RewindAll(false);
	});
	Add(TEXT("v_toggle_x11"), 1.2f, [this] { for (int32 i = 0; i < 11; ++i) { Powers->ToggleOverdrive(); } });
	Add(TEXT("v_f_r_juntos"), 1.5f, [this]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		if (Powers->InverseStep) { Powers->InverseStep->BeginInverseStep(); }
		if (Powers->Inversion) { Powers->Inversion->BeginInversion(); }
		Rooms->Fire();
	});
	Add(TEXT("soltar_todo"), 0.8f, [this]
	{
		if (Powers->InverseStep) { Powers->InverseStep->EndInverseStep(); }
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
	});
	Add(TEXT("spam_q_x8"), 2.5f, [this] { for (int32 i = 0; i < 8; ++i) { Powers->SpawnBubble(); } Rooms->Fire(); });
	Add(TEXT("sin_energia_f_e_v"), 1.f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = 0.f; }
		if (Powers->InverseStep) { Powers->InverseStep->BeginInverseStep(); }
		Powers->CreateEcho();
		Powers->ToggleOverdrive();
		Powers->SpawnBubble();
	});
	Add(TEXT("spam_e_x6_g"), 1.5f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		for (int32 i = 0; i < 6; ++i) { Powers->CreateEcho(); }
		Powers->ClearEchoes();
		Powers->CreateEcho();
	});
	Add(TEXT("teletransportes_x5"), 1.2f, [this] { Rooms->GoToRoom(1); Rooms->GoToRoom(2); Rooms->GoToRoom(3); Rooms->GoToRoom(4); Rooms->GoToRoom(0); Rooms->GoToRoom(4); });
	Add(TEXT("x_y_r_a_la_bala"), 2.f, [this]
	{
		Rooms->FireInverted();
		Rooms->Fire();
		ATemporalProjectile* Newest = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It)
		{
			if (!Newest || It->GetGameTimeSinceCreation() < Newest->GetGameTimeSinceCreation())
			{
				Newest = *It;
			}
		}
		if (Newest && Powers->Inversion) { Powers->Inversion->BeginInversionOn(Newest->Recorder); }
	});
	Add(TEXT("rebobinar_largo"), 4.f, [this]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		Rooms->RewindAll(true);
	});
	Add(TEXT("fin_rebobinar"), 1.f, [this] { Rooms->RewindAll(false); });
	Add(TEXT("churn_romper_rearmar"), 2.5f, [this, BreakAll]
	{
		for (int32 i = 0; i < 4; ++i)
		{
			BreakAll();
			for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It) { It->Restore(); }
		}
		BreakAll();
	});
	Add(TEXT("t_cajas_x5"), 1.5f, [this] { for (int32 i = 0; i < 5; ++i) { Rooms->SpawnCubes(); } });
	// Fase 3 · Detener: más de 3 a la vez, cosas que desaparecen detenidas, invertir algo detenido, burbuja encima.
	Add(TEXT("detener_x6_y_borrar"), 1.5f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		Rooms->GoToRoom(4);
		for (int32 i = 0; i < 3; ++i) { Rooms->Fire(); }
		int32 Count = 0;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It && Count < 6; ++It, ++Count)
		{
			if (Powers->TimeStop) { Powers->TimeStop->StopActor(*It); }
		}
		for (AActor* Cube : Rooms->Cubes)
		{
			if (Powers->TimeStop && IsValid(Cube)) { Powers->TimeStop->StopActor(Cube); }
		}
		if (Rooms->PhysicsLab && Rooms->PhysicsLab->GetTurret()) { Rooms->PhysicsLab->GetTurret()->ClearBullets(); }
	});
	Add(TEXT("detener_y_r"), 1.5f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		Rooms->Fire();
		ATemporalProjectile* Newest = nullptr;
		for (TActorIterator<ATemporalProjectile> It(GetWorld()); It; ++It) { Newest = *It; }
		if (Newest && Powers->TimeStop) { Powers->TimeStop->StopActor(Newest); }
		if (Newest && Powers->Inversion) { Powers->Inversion->BeginInversionOn(Newest->Recorder); }
		Powers->SpawnBubble();
	});
	Add(TEXT("detener_suelta_todo"), 1.f, [this]
	{
		if (Powers->Inversion) { Powers->Inversion->EndInversion(); }
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
	});
	// Fase 3 · Dash: muchos seguidos, con Sobremarcha, y el Paso Inverso a medio dash.
	Add(TEXT("spam_dash_x10"), 1.f, [this, Player]
	{
		if (Player()) { Player()->Jump(); }
		for (int32 i = 0; i < 10; ++i) { Powers->TryDash(); }
	});
	Add(TEXT("dash_v_y_f"), 1.2f, [this]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		Powers->TryDash();
		if (Powers->InverseStep) { Powers->InverseStep->BeginInverseStep(); }
	});
	Add(TEXT("dash_suelta"), 0.8f, [this]
	{
		if (Powers->InverseStep) { Powers->InverseStep->EndInverseStep(); }
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
		Powers->TryDash();
	});
	// Fase 3 · Todos los tiempos lentos encimados y luego soltar todo: el mundo y el jugador deben volver a 1.
	Add(TEXT("todo_lento_junto"), 1.5f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		if (Powers->Dash) { Powers->Dash->StartReflex(); }
		Powers->SpawnBubble();
		for (int32 i = 0; i < 3; ++i) { Rooms->Fire(); }
	});
	Add(TEXT("todo_lento_suelta"), 2.f, [this]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
	});
	Add(TEXT("todo_lento_revisa"), 0.5f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR todo_lento global=%.2f jugador=%.2f"),
			Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
	});
	// Fase 3 · Ojo lento + burbuja: al dejar de apuntar, las balas siguen congeladas por la burbuja (gana la más lenta).
	Add(TEXT("ojo_lento_y_burbuja"), 0.3f, [this, Player]
	{
		ACharacter* Character = Player();
		if (!Character)
		{
			return;
		}
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
		const FVector Forward = Character->GetActorForwardVector();
		const FVector Right = Character->GetActorRightVector();
		const FVector Chest = Character->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		SpawnTourBullet(Chest + Forward * 250.f - Right * 150.f, Right);
		SpawnTourBullet(Chest + Forward * 300.f - Right * 200.f, Right);
	});
	Add(TEXT("ojo_lento_burbuja"), 0.6f, [this] { Powers->SpawnBubble(); });
	Add(TEXT("ojo_lento_suelta"), 0.6f, [this] { if (Powers->Aim) { Powers->Aim->SetAiming(false); } });
	Add(TEXT("ojo_lento_revisa_burbuja"), 0.5f, [this]
	{
		float Fastest = -1.f;
		for (const TWeakObjectPtr<AActor>& Temp : TourTemps)
		{
			if (Temp.IsValid()) { Fastest = FMath::Max(Fastest, Temp->CustomTimeDilation); }
		}
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_lento_burbuja bala_mas_rapida=%.2f (0 = sigue congelada)"), Fastest);
		ClearTourTemps();
	});
	// Fase 3 · Morir a media acción: F, R, Sobremarcha, apuntar y dash a la vez.
	Add(TEXT("sala5_caos"), 1.f, [this] { Rooms->GoToRoom(5); });
	Add(TEXT("morir_en_todo"), 1.2f, [this]
	{
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
		if (Powers->InverseStep) { Powers->InverseStep->BeginInverseStep(); }
		if (Powers->Inversion) { Powers->Inversion->BeginInversion(); }
		if (Powers->Health) { Powers->Health->ApplyDamage(1000.f); }
		Powers->TryDash();
	});
	Add(TEXT("despues_de_morir"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR despues_de_morir vida=%.0f global=%.2f jugador=%.2f apuntando=%d"),
			Powers->Health ? Powers->Health->Health : -1.f, Dilation ? Dilation->GetGlobalDilation() : -1.f,
			Pawn ? Pawn->CustomTimeDilation : -1.f, Powers->Aim && Powers->Aim->IsAiming() ? 1 : 0);
	});
	// Fase 3 · Remates raros: muchos clics, dash y Sobremarcha a la vez, y teletransportarse a medio remate.
	Add(TEXT("remate_spam"), 1.f, [this]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(30.f); }
		if (Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().Num() > 0)
		{
			if (ATemporalMannequin* Target = Rooms->ReflexRoom->GetMannequins()[0])
			{
				if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
				{
					Pawn->SetActorLocation(Target->GetActorLocation() - Target->GetActorForwardVector() * 120.f, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
	});
	Add(TEXT("remate_x5_dash_v"), 1.f, [this]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		for (int32 i = 0; i < 5; ++i) { Powers->PrimaryAction(); Powers->TryDash(); }
	});
	Add(TEXT("remate_y_teletransporte"), 1.f, [this]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
		Powers->PrimaryAction();
		Rooms->GoToRoom(4);
	});
	// Revisión final · Ojo lento en Sobremarcha: la bala va a 0.25 (gana la más lenta), no a 0.25 × 0.25.
	Add(TEXT("ojo_en_sobremarcha"), 0.3f, [this]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(30.f); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(true); }
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector Forward = Pawn->GetActorForwardVector();
			const FVector Right = Pawn->GetActorRightVector();
			SpawnTourBullet(Pawn->GetActorLocation() + FVector(0.f, 0.f, 20.f) + Forward * 250.f - Right * 150.f, Right);
		}
	});
	Add(TEXT("ojo_en_sobremarcha_revisa"), 0.3f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const AActor* Bullet = TourTemps.Num() > 0 ? TourTemps.Last().Get() : nullptr;
		const float Rate = Bullet && Dilation ? Bullet->CustomTimeDilation * Dilation->GetGlobalDilation() : -1.f;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR ojo_en_sobremarcha ritmo_bala=%.3f (esperado 0.250)"), Rate);
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
		if (Powers->Aim) { Powers->Aim->SetAiming(false); }
		ClearTourTemps();
	});
	// Revisión final · Detener a un maniquí que va cayendo: el cuerpo debe quedarse quieto.
	auto StandBehind = [this](int32 Index, float Distance)
	{
		ATemporalMannequin* Target = Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().IsValidIndex(Index) ? Rooms->ReflexRoom->GetMannequins()[Index].Get() : nullptr;
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Target && Pawn)
		{
			Pawn->SetActorLocation(Target->GetActorLocation() - Target->GetActorForwardVector() * Distance, false, nullptr, ETeleportType::TeleportPhysics);
		}
		return Target;
	};
	Add(TEXT("remate_para_detener_cayendo"), 0.35f, [this, StandBehind]
	{
		Rooms->GoToRoom(5);
		if (Powers->Health) { Powers->Health->SetInvulnerable(30.f); }
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (ATemporalMannequin* Target = StandBehind(0, 120.f))
		{
			Target->ReceiveTakedown((Target->GetActorForwardVector() + FVector(0.f, 0.f, 0.35f)).GetSafeNormal(), 900.f);
		}
	});
	Add(TEXT("detener_cayendo"), 0.15f, [this]
	{
		ATemporalMannequin* Target = Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().Num() > 0 ? Rooms->ReflexRoom->GetMannequins()[0].Get() : nullptr;
		if (Target && Powers->TimeStop)
		{
			Powers->TimeStop->StopActor(Target);
		}
	});
	// La medida empieza un momento después: el congelado se aplica en el siguiente frame del maniquí.
	Add(TEXT("detener_cayendo_marca"), 0.4f, [this]
	{
		ATemporalMannequin* Target = Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().Num() > 0 ? Rooms->ReflexRoom->GetMannequins()[0].Get() : nullptr;
		if (Target)
		{
			TourMark = Target->GetMesh()->GetBoneLocation(TEXT("pelvis"));
		}
	});
	Add(TEXT("detener_cayendo_revisa"), 0.3f, [this]
	{
		ATemporalMannequin* Target = Rooms->ReflexRoom && Rooms->ReflexRoom->GetMannequins().Num() > 0 ? Rooms->ReflexRoom->GetMannequins()[0].Get() : nullptr;
		const float Moved = Target ? FVector::Dist(TourMark, Target->GetMesh()->GetBoneLocation(TEXT("pelvis"))) : -1.f;
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR maniqui_cayendo_detenido se_movio=%.1f cm tirado=%d"), Moved, Target && Target->IsKnockedDown() ? 1 : 0);
		if (Powers->TimeStop) { Powers->TimeStop->ReleaseAll(); }
	});
	// Revisión final · Morir con todo activo (dash, reflejo, algo detenido, remate): al reaparecer no queda nada.
	Add(TEXT("morir_con_todo_activo"), 0.2f, [this, StandBehind]
	{
		Rooms->GoToRoom(5);
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		StandBehind(0, 120.f);
		Powers->TryDash();
		if (Powers->Dash) { Powers->Dash->StartReflex(); }
		if (Powers->TimeStop && Rooms->ReflexRoom && Rooms->ReflexRoom->GetTurrets().Num() > 0) { Powers->TimeStop->StopActor(Rooms->ReflexRoom->GetTurrets()[0]); }
		Powers->PrimaryAction();
		if (Powers->Health)
		{
			Powers->Health->ClearInvulnerable();
			Powers->Health->ApplyDamage(1000.f);
		}
	});
	Add(TEXT("morir_con_todo_revisa"), 0.5f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tras_morir_con_todo global=%.2f detenidos=%d dash=%d reflejo=%d remate=%d"),
			Dilation ? Dilation->GetGlobalDilation() : -1.f, Powers->TimeStop ? Powers->TimeStop->NumStopped() : -1,
			Powers->Dash && Powers->Dash->IsDashing() ? 1 : 0, Powers->Dash && Powers->Dash->IsReflexActive() ? 1 : 0,
			Powers->Takedown && Powers->Takedown->IsBusy() ? 1 : 0);
	});
	Add(TEXT("remate_revisa"), 1.f, [this]
	{
		const UTemporalDilationSubsystem* Dilation = UTemporalDilationSubsystem::Get(this);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR remate_revisa global=%.2f jugador=%.2f"), Dilation ? Dilation->GetGlobalDilation() : -1.f, Pawn ? Pawn->CustomTimeDilation : -1.f);
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); }
	});
	// Fase 3b · Parkour con todo encima: F a medio salto, morir colgado, teletransportarse colgado, dash colgado.
	auto Movement = [this]()
	{
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		return Character ? static_cast<int32>(Character->GetCharacterMovement()->MovementMode) : -1;
	};
	Add(TEXT("parkour_y_f_prepara"), 0.8f, [this]
	{
		Rooms->GoToRoom(6);
		if (Rooms->ParkourRoom) { Rooms->ParkourRoom->PlacePlayer(FVector(490.f, -450.f, 98.f)); }
	});
	Add(TEXT("parkour_y_f"), 0.3f, [this]
	{
		if (ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { Character->Jump(); }
		const bool bVaulting = Powers->Parkour && Powers->Parkour->IsBusy();
		const bool bInverse = Powers->InverseStep && Powers->InverseStep->BeginInverseStep();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR parkour_y_f saltando=%d paso_inverso=%d"), bVaulting ? 1 : 0, bInverse ? 1 : 0);
	});
	Add(TEXT("parkour_y_f_suelta"), 0.6f, [this]
	{
		if (Powers->InverseStep) { Powers->InverseStep->EndInverseStep(); }
	});
	Add(TEXT("parkour_y_f_revisa"), 0.2f, [this, Movement]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR parkour_y_f_fin ocupado=%d modo=%d"), Powers->Parkour && Powers->Parkour->IsBusy() ? 1 : 0, Movement());
	});
	auto HangAgain = [this]()
	{
		if (Rooms->ParkourRoom)
		{
			Rooms->ParkourRoom->PlacePlayer(FVector(600.f, 600.f, 98.f));
			if (Powers->Locomotion) { Powers->Locomotion->SetTourInput(Rooms->ParkourRoom->GetActorForwardVector()); }
		}
		if (ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { Character->Jump(); }
	};
	Add(TEXT("colgado_y_dash_v"), 1.f, [HangAgain] { HangAgain(); });
	Add(TEXT("colgado_dash_v"), 0.6f, [this]
	{
		Powers->TryDash();
		if (Powers->Overdrive) { Powers->Overdrive->StartOverdrive(); }
	});
	Add(TEXT("colgado_teletransporte"), 0.6f, [this, Movement]
	{
		if (Powers->Overdrive) { Powers->Overdrive->StopOverdrive(); }
		const bool bWasHanging = Powers->Parkour && Powers->Parkour->IsHanging();
		Rooms->GoToRoom(4);
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR colgado_teletransporte estaba=%d ocupado=%d modo=%d"), bWasHanging ? 1 : 0,
			Powers->Parkour && Powers->Parkour->IsBusy() ? 1 : 0, Movement());
	});
	Add(TEXT("colgado_y_morir"), 1.f, [this, HangAgain] { Rooms->GoToRoom(6); HangAgain(); });
	Add(TEXT("colgado_muere"), 0.5f, [this]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR antes_de_morir colgado=%d"), Powers->Parkour && Powers->Parkour->IsHanging() ? 1 : 0);
		if (Powers->Locomotion) { Powers->Locomotion->SetTourInput(FVector::ZeroVector); }
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); Powers->Health->ApplyDamage(1000.f); }
	});
	Add(TEXT("colgado_muere_revisa"), 0.5f, [this, Movement]
	{
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR tras_morir_colgado ocupado=%d modo=%d vida=%.0f"),
			Powers->Parkour && Powers->Parkour->IsBusy() ? 1 : 0, Movement(), Powers->Health ? Powers->Health->Health : -1.f);
	});
	// Fase 3b · Cobertura que se rompe: cubierto detrás de la caja rompible de la sala 4, se rompe y sales sin quedar pegado.
	Add(TEXT("cobertura_rompible"), 0.6f, [this]
	{
		Rooms->GoToRoom(4);
		if (Rooms->PhysicsLab)
		{
			if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				const FVector Spot = Rooms->PhysicsLab->GetActorTransform().TransformPosition(FVector(1000.f, -400.f, 98.f));
				Pawn->SetActorLocationAndRotation(Spot, FRotator(0.f, Rooms->PhysicsLab->GetActorRotation().Yaw - 90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		const bool bIn = Powers->Cover && Powers->Cover->TryEnter();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR cobertura_rompible entro=%d"), bIn ? 1 : 0);
	});
	Add(TEXT("cobertura_se_rompe"), 0.6f, [this]
	{
		for (TActorIterator<ATemporalBreakable> It(GetWorld()); It; ++It)
		{
			It->Break(It->GetActorLocation(), FVector::UpVector);
		}
	});
	Add(TEXT("cobertura_rompible_revisa"), 0.3f, [this]
	{
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR cobertura_rota cubierto=%d plano=%d"), Powers->Cover && Powers->Cover->IsInCover() ? 1 : 0,
			Character && Character->GetCharacterMovement()->bConstrainToPlane ? 1 : 0);
	});
	// Fase 3b · Agarre raro: agarrar a un congelado, detenerlo en la mano, morir agarrando y teletransportarse agarrando.
	auto GrabDummy = [this](int32 Index)
	{
		ATemporalArmedDummy* Target = Rooms->CombatRoom && Rooms->CombatRoom->GetDummies().IsValidIndex(Index) ? Rooms->CombatRoom->GetDummies()[Index].Get() : nullptr;
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Target && Pawn)
		{
			const FVector Spot = Target->GetActorLocation() - Target->GetActorForwardVector() * 110.f;
			Pawn->SetActorLocationAndRotation(Spot, FRotator(0.f, Target->GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		return Target;
	};
	Add(TEXT("agarre_congelado"), 0.6f, [this, GrabDummy]
	{
		Rooms->GoToRoom(7);
		if (Powers->Energy) { Powers->Energy->Energy = Powers->Energy->MaxEnergy; }
		if (ATemporalArmedDummy* Target = GrabDummy(0))
		{
			if (Powers->TimeStop) { Powers->TimeStop->StopActor(Target); }
		}
		const bool bOk = Powers->Grab && Powers->Grab->TryGrab();
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR agarre_congelado ok=%d"), bOk ? 1 : 0);
	});
	Add(TEXT("agarre_y_muere"), 0.5f, [this]
	{
		if (Powers->Health) { Powers->Health->ClearInvulnerable(); Powers->Health->ApplyDamage(1000.f); }
	});
	Add(TEXT("agarre_y_muere_revisa"), 0.4f, [this]
	{
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR agarre_tras_morir agarrando=%d modo=%d detenidos=%d"), Powers->Grab && Powers->Grab->IsHolding() ? 1 : 0,
			Character ? static_cast<int32>(Character->GetCharacterMovement()->MovementMode) : -1, Powers->TimeStop ? Powers->TimeStop->NumStopped() : -1);
	});
	Add(TEXT("agarre_y_teletransporte"), 0.4f, [this, GrabDummy]
	{
		GrabDummy(1);
		if (Powers->Grab) { Powers->Grab->TryGrab(); }
		Rooms->GoToRoom(4);
	});
	Add(TEXT("agarre_teletransporte_revisa"), 0.4f, [this]
	{
		const ACharacter* Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		UE_LOG(LogTemp, Display, TEXT("ENTROPIA_TOUR agarre_tras_teletransporte agarrando=%d modo=%d"), Powers->Grab && Powers->Grab->IsHolding() ? 1 : 0,
			Character ? static_cast<int32>(Character->GetCharacterMovement()->MovementMode) : -1);
	});
	Add(TEXT("calma"), 2.5f, [] {});
}
