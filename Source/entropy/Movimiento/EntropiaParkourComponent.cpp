// ENTROPÍA · Parkour

#include "EntropiaParkourComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaLocomotionComponent.h"
#include "EntropiaCoverComponent.h"
#include "InverseStepComponent.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace EntropiaParkourProbe
{
	/** Altura (desde los pies) del trazo que busca obstáculos. Más bajo que MinHeight + margen. */
	constexpr float KneeHeight = 60.f;
	/** Paso con el que se busca dónde termina el obstáculo. */
	constexpr float DepthStep = 20.f;
	constexpr float DepthLimit = 260.f;
}

UEntropiaParkourComponent::UEntropiaParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

ACharacter* UEntropiaParkourComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

FVector UEntropiaParkourComponent::InputDirection() const
{
	const ACharacter* Character = GetCharacter();
	FVector Direction = Character->GetPendingMovementInputVector();
	if (Direction.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		Direction = Character->GetLastMovementInputVector();
	}
	Direction.Z = 0.f;
	return Direction.SizeSquared() < KINDA_SMALL_NUMBER ? FVector::ZeroVector : Direction.GetSafeNormal();
}

FVector UEntropiaParkourComponent::IntentDirection() const
{
	const ACharacter* Character = GetCharacter();
	FVector Direction = Character->GetPendingMovementInputVector();
	if (Direction.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		Direction = Character->GetLastMovementInputVector();
	}
	if (Direction.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		Direction = Character->GetActorForwardVector();
	}
	Direction.Z = 0.f;
	return Direction.GetSafeNormal();
}

FEntropiaObstacle UEntropiaParkourComponent::Measure(FVector& OutEdge, FVector& OutTop, FVector& OutLanding, FVector& OutInto) const
{
	FEntropiaObstacle Result;
	const ACharacter* Character = GetCharacter();
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return Result;
	}
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Center = Character->GetActorLocation();
	const float FeetZ = Center.Z - HalfHeight;
	const FVector Forward = IntentDirection();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaParkour), false, Character);

	// 1) ¿Hay algo enfrente a la altura de las rodillas?
	const FVector KneeStart(Center.X, Center.Y, FeetZ + EntropiaParkourProbe::KneeHeight);
	FHitResult Face;
	if (!World->LineTraceSingleByChannel(Face, KneeStart, KneeStart + Forward * (Radius + EntropiaParkour::MaxDistance), ECC_Visibility, Params)
		|| (Face.GetActor() && Face.GetActor()->IsA<APawn>())
		|| FMath::Abs(Face.ImpactNormal.Z) > 0.5f)
	{
		return Result;
	}
	FVector Into = -Face.ImpactNormal;
	Into.Z = 0.f;
	Into = Into.GetSafeNormal();
	OutInto = Into;
	Result.bHit = true;
	Result.Distance = FMath::Max(0.f, Face.Distance - Radius);

	// 2) ¿Qué tan alto es? Trazo hacia abajo justo detrás de la cara.
	const FVector Inside = Face.ImpactPoint + Into * 10.f;
	const float ProbeTop = FeetZ + EntropiaParkour::WallRunMaxHeight + 40.f;
	FHitResult Top;
	const bool bTop = World->LineTraceSingleByChannel(Top, FVector(Inside.X, Inside.Y, ProbeTop), FVector(Inside.X, Inside.Y, FeetZ + 5.f), ECC_Visibility, Params);
	if (!bTop || Top.bStartPenetrating)
	{
		// Más alto que todo lo que se puede trepar (o un hueco raro).
		Result.Height = bTop ? 999.f : 0.f;
		return Result;
	}
	const float TopZ = Top.ImpactPoint.Z;
	Result.Height = TopZ - FeetZ;
	OutEdge = FVector(Face.ImpactPoint.X, Face.ImpactPoint.Y, TopZ);

	// 3) ¿Cuánto fondo tiene? Avanzar por encima hasta que ya no haya "techo" del obstáculo.
	Result.Depth = 999.f;
	for (float D = EntropiaParkourProbe::DepthStep; D <= EntropiaParkourProbe::DepthLimit; D += EntropiaParkourProbe::DepthStep)
	{
		const FVector Q = Face.ImpactPoint + Into * D;
		FHitResult Sample;
		if (!World->LineTraceSingleByChannel(Sample, FVector(Q.X, Q.Y, TopZ + 30.f), FVector(Q.X, Q.Y, TopZ - 40.f), ECC_Visibility, Params))
		{
			Result.Depth = D - EntropiaParkourProbe::DepthStep * 0.5f;
			break;
		}
	}

	// 4) ¿Cabes de pie arriba?
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	const FVector TopStand = Face.ImpactPoint + Into * (Radius + 15.f) + FVector(0.f, 0.f, TopZ - Face.ImpactPoint.Z + HalfHeight + 2.f);
	Result.bTopClear = !World->OverlapBlockingTestByChannel(TopStand, FQuat::Identity, ECC_Pawn, Shape, Params);
	OutTop = TopStand;

	// 5) ¿Hay piso y espacio del otro lado? (para pasar por encima)
	if (Result.Depth < 999.f)
	{
		const FVector Beyond = Face.ImpactPoint + Into * (Result.Depth + Radius + 40.f);
		FHitResult Floor;
		if (World->LineTraceSingleByChannel(Floor, FVector(Beyond.X, Beyond.Y, TopZ + 50.f), FVector(Beyond.X, Beyond.Y, FeetZ - 200.f), ECC_Visibility, Params)
			&& Floor.ImpactPoint.Z < TopZ - 30.f)
		{
			const FVector Landing(Beyond.X, Beyond.Y, Floor.ImpactPoint.Z + HalfHeight + 2.f);
			Result.bLandingClear = !World->OverlapBlockingTestByChannel(Landing, FQuat::Identity, ECC_Pawn, Shape, Params);
			if (Result.bLandingClear)
			{
				OutLanding = Landing;
			}
		}
	}
	return Result;
}

bool UEntropiaParkourComponent::TryStart()
{
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}
	if (bHanging)
	{
		// Colgado: Espacio sube (si hay espacio arriba).
		TryClimbFromHang();
		return true;
	}
	if (IsBusy())
	{
		// A medio movimiento, Espacio no hace nada (ni salta).
		return true;
	}
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if ((Inverse && Inverse->IsInverseActive()) || !Move)
	{
		return false;
	}
	if (!Move->IsMovingOnGround())
	{
		// En el aire, Espacio agarra una orilla si la tienes enfrente.
		return Move->IsFalling() && TryGrabLedge(false);
	}

	if (UEntropiaCoverComponent* Cover = Character->FindComponentByClass<UEntropiaCoverComponent>(); Cover && Cover->IsInCover())
	{
		Cover->Exit();
	}
	UEntropiaLocomotionComponent* Locomotion = Character->FindComponentByClass<UEntropiaLocomotionComponent>();
	if (Locomotion && Locomotion->IsCrouched())
	{
		Locomotion->SetCrouched(false);
	}

	FVector Edge, Top, Landing, Into;
	const FEntropiaObstacle Obstacle = Measure(Edge, Top, Landing, Into);
	const bool bSprinting = Locomotion && Locomotion->IsSprinting() && Character->GetVelocity().Size2D() > 300.f;
	const EEntropiaTraversal Kind = EntropiaParkour::Classify(Obstacle, bSprinting);
	if (Kind == EEntropiaTraversal::None)
	{
		return false;
	}

	Current = Kind;
	Elapsed = 0.f;
	Duration = EntropiaParkour::Duration(Kind);
	PathStart = Character->GetActorLocation();
	PathEdge = Edge;
	PathEnd = (Kind == EEntropiaTraversal::DashVault || Kind == EEntropiaTraversal::Kong) ? Landing : Top;
	ExitDirection = Into;
	Character->SetActorRotation(FRotator(0.f, Into.Rotation().Yaw, 0.f));
	Move->StopMovementImmediately();
	Move->DisableMovement();

	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		const FEntropiaAnimSlot* Slot = nullptr;
		switch (Kind)
		{
		case EEntropiaTraversal::DashVault: Slot = &Set->DashVault; break;
		case EEntropiaTraversal::Kong: Slot = &Set->Kong; break;
		case EEntropiaTraversal::StepUp: Slot = &Set->StepUp; break;
		case EEntropiaTraversal::Climb: Slot = &Set->Climb; break;
		case EEntropiaTraversal::WallRun: Slot = &Set->WallRun; break;
		default: break;
		}
		if (Slot)
		{
			EntropiaAnim::Play(Character, *Slot);
		}
	}
	return true;
}

void UEntropiaParkourComponent::Finish()
{
	ACharacter* Character = GetCharacter();
	const EEntropiaTraversal Done = Current;
	Current = EEntropiaTraversal::None;
	Last = Done;
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(MOVE_Walking);
	// Al pasar por encima sigues con impulso; al subir o trepar quedas quieto arriba.
	float ExitSpeed = 0.f;
	if (Done == EEntropiaTraversal::DashVault)
	{
		ExitSpeed = VaultExitSpeed;
	}
	else if (Done == EEntropiaTraversal::Kong)
	{
		ExitSpeed = KongExitSpeed;
	}
	Move->Velocity = ExitDirection * ExitSpeed;
	EntropiaAnim::Stop(Character, 0.2f);
}

bool UEntropiaParkourComponent::TryGrabLedge(bool bNeedIntent)
{
	ACharacter* Character = GetCharacter();
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	UWorld* World = GetWorld();
	if (!Capsule || !World || IsBusy() || GrabCooldown > 0.f)
	{
		return false;
	}
	const FVector Input = InputDirection();
	if (bNeedIntent && Input.IsNearlyZero())
	{
		return false;
	}
	const FVector Forward = Input.IsNearlyZero() ? IntentDirection() : Input;
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Center = Character->GetActorLocation();
	const float FeetZ = Center.Z - HalfHeight;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaLedge), false, Character);

	// Una pared enfrente, a la altura del pecho.
	const FVector Chest = Center + FVector(0.f, 0.f, HalfHeight * 0.5f);
	FHitResult Face;
	if (!World->LineTraceSingleByChannel(Face, Chest, Chest + Forward * (Radius + 60.f), ECC_Visibility, Params)
		|| (Face.GetActor() && Face.GetActor()->IsA<APawn>()) || FMath::Abs(Face.ImpactNormal.Z) > 0.5f)
	{
		return false;
	}
	FVector Into = -Face.ImpactNormal;
	Into.Z = 0.f;
	Into = Into.GetSafeNormal();
	if (bNeedIntent && FVector::DotProduct(Input, Into) < 0.5f)
	{
		return false;
	}

	// Su borde, al alcance de las manos.
	const FVector Inside = Face.ImpactPoint + Into * 10.f;
	FHitResult Top;
	if (!World->LineTraceSingleByChannel(Top, FVector(Inside.X, Inside.Y, FeetZ + EntropiaParkour::LedgeMaxHeight + 30.f),
		FVector(Inside.X, Inside.Y, FeetZ + EntropiaParkour::LedgeMinHeight - 30.f), ECC_Visibility, Params) || Top.bStartPenetrating)
	{
		return false;
	}
	if (!EntropiaParkour::CanGrabLedge(Top.ImpactPoint.Z - FeetZ))
	{
		return false;
	}
	const FVector Edge(Face.ImpactPoint.X, Face.ImpactPoint.Y, Top.ImpactPoint.Z);
	const FVector HangPos = Edge - Into * (Radius + 5.f) - FVector(0.f, 0.f, EntropiaParkour::HangDrop);
	if (World->OverlapBlockingTestByChannel(HangPos, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
	{
		return false;
	}
	StartHang(Edge, Into);
	return true;
}

void UEntropiaParkourComponent::StartHang(const FVector& Edge, const FVector& Into)
{
	ACharacter* Character = GetCharacter();
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	bHanging = true;
	LedgeEdge = Edge;
	LedgeInto = Into;
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	Character->SetActorLocationAndRotation(Edge - Into * (Radius + 5.f) - FVector(0.f, 0.f, EntropiaParkour::HangDrop),
		FRotator(0.f, Into.Rotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(Character, Set->Hang);
	}
}

bool UEntropiaParkourComponent::LedgeContinuesAt(const FVector& EdgePoint) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaLedgeAlong), false, GetCharacter());
	const FVector Inside = EdgePoint + LedgeInto * 10.f;
	FHitResult Top;
	return GetWorld()->LineTraceSingleByChannel(Top, Inside + FVector(0.f, 0.f, 40.f), Inside - FVector(0.f, 0.f, 40.f), ECC_Visibility, Params)
		&& !Top.bStartPenetrating && FMath::Abs(Top.ImpactPoint.Z - EdgePoint.Z) < 25.f;
}

void UEntropiaParkourComponent::UpdateHang(float DeltaTime)
{
	ACharacter* Character = GetCharacter();
	const FVector Input = InputDirection();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, LedgeInto).GetSafeNormal();
	const float Side = FVector::DotProduct(Input, Right);
	if (FMath::Abs(Side) < 0.3f)
	{
		return;
	}
	// De lado, mientras siga habiendo orilla y no choques con nada.
	const FVector Step = Right * FMath::Sign(Side) * ShimmySpeed * DeltaTime;
	const FVector NewEdge = LedgeEdge + Step;
	if (!LedgeContinuesAt(NewEdge + Right * FMath::Sign(Side) * Character->GetCapsuleComponent()->GetScaledCapsuleRadius()))
	{
		return;
	}
	FHitResult Blocked;
	Character->SetActorLocation(Character->GetActorLocation() + Step, true, &Blocked);
	if (!Blocked.bBlockingHit)
	{
		LedgeEdge = NewEdge;
	}
}

bool UEntropiaParkourComponent::TryClimbFromHang()
{
	ACharacter* Character = GetCharacter();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector TopStand = LedgeEdge + LedgeInto * (Radius + 15.f) + FVector(0.f, 0.f, HalfHeight + 2.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EntropiaLedgeClimb), false, Character);
	if (GetWorld()->OverlapBlockingTestByChannel(TopStand, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
	{
		return false;
	}
	bHanging = false;
	Current = EEntropiaTraversal::HangClimb;
	Elapsed = 0.f;
	Duration = EntropiaParkour::Duration(Current);
	PathStart = Character->GetActorLocation();
	PathEdge = LedgeEdge;
	PathEnd = TopStand;
	ExitDirection = LedgeInto;
	if (const UEntropiaAnimationSet* Set = UEntropiaAnimationSet::Get())
	{
		EntropiaAnim::Play(Character, Set->ClimbFromHang);
	}
	return true;
}

void UEntropiaParkourComponent::Drop()
{
	if (!bHanging)
	{
		return;
	}
	bHanging = false;
	GrabCooldown = 0.5f;
	ACharacter* Character = GetCharacter();
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(MOVE_Falling);
	Move->Velocity = -LedgeInto * 150.f;
	EntropiaAnim::Stop(Character, 0.2f);
}

void UEntropiaParkourComponent::Cancel()
{
	if (bHanging)
	{
		bHanging = false;
		GrabCooldown = 0.5f;
		if (ACharacter* Character = GetCharacter())
		{
			const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
			if (!(Inverse && Inverse->IsInverseActive()))
			{
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			}
			EntropiaAnim::Stop(Character, 0.2f);
		}
		return;
	}
	if (!IsBusy())
	{
		return;
	}
	Last = Current;
	Current = EEntropiaTraversal::None;
	if (ACharacter* Character = GetCharacter())
	{
		const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
		// Durante el Paso Inverso el movimiento lo maneja él (lo reactiva al terminar).
		if (!(Inverse && Inverse->IsInverseActive()))
		{
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	}
}

void UEntropiaParkourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	GrabCooldown = FMath::Max(0.f, GrabCooldown - DeltaTime);
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	if (!IsBusy())
	{
		// Cayendo hacia un borde: te cuelgas solo si empujas el stick hacia él.
		const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		if (Move && Move->IsFalling())
		{
			TryGrabLedge(true);
		}
		return;
	}
	if (bHanging)
	{
		const UInverseStepComponent* InverseNow = Character->FindComponentByClass<UInverseStepComponent>();
		if (InverseNow && InverseNow->IsInverseActive())
		{
			Cancel();
			return;
		}
		UpdateHang(DeltaTime);
		return;
	}
	const UInverseStepComponent* Inverse = Character ? Character->FindComponentByClass<UInverseStepComponent>() : nullptr;
	if (!Character || (Inverse && Inverse->IsInverseActive()))
	{
		Cancel();
		return;
	}

	Elapsed += DeltaTime;
	const float Alpha = Duration > 0.f ? Elapsed / Duration : 1.f;
	Character->SetActorLocation(EntropiaParkour::PathPoint(Current, PathStart, PathEdge, PathEnd, Alpha), false, nullptr, ETeleportType::TeleportPhysics);
	if (Alpha >= 1.f)
	{
		Finish();
	}
}
