// ENTROPÍA · Parkour

#include "EntropiaParkourComponent.h"
#include "EntropiaAnimationSet.h"
#include "EntropiaLocomotionComponent.h"
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
	if (IsBusy())
	{
		// A medio movimiento, Espacio no hace nada (ni salta).
		return true;
	}
	const UInverseStepComponent* Inverse = Character->FindComponentByClass<UInverseStepComponent>();
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if ((Inverse && Inverse->IsInverseActive()) || !Move || !Move->IsMovingOnGround())
	{
		return false;
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

void UEntropiaParkourComponent::Cancel()
{
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
	if (!IsBusy())
	{
		return;
	}
	ACharacter* Character = GetCharacter();
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
