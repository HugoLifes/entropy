// ENTROPÍA · Apuntar y mira

#include "TemporalAimComponent.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalSubsystem.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UTemporalAimComponent::UTemporalAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTemporalAimComponent::BeginPlay()
{
	Super::BeginPlay();
	HighlightMaterial = TemporalFX::MakeTranslucent(this, HighlightColor);
	Boom = GetOwner()->FindComponentByClass<USpringArmComponent>();

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		BaseWalkSpeed = Move->MaxWalkSpeed;
		bBaseOrientToMovement = Move->bOrientRotationToMovement;
		bBaseUseControllerDesiredRotation = Move->bUseControllerDesiredRotation;
		BaseRotationRate = Move->RotationRate;
	}
	ApplyCamera();
}

void UTemporalAimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHighlight();
	Super::EndPlay(EndPlayReason);
}

void UTemporalAimComponent::SetAiming(bool bAim)
{
	if (bAiming == bAim)
	{
		return;
	}
	bAiming = bAim;

	// Apuntando, el personaje gira hacia la mira (no hacia donde camina): así puedes moverte de lado sin dejar de apuntar.
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		Move->bOrientRotationToMovement = bAiming ? false : bBaseOrientToMovement;
		Move->bUseControllerDesiredRotation = bAiming ? true : bBaseUseControllerDesiredRotation;
		Move->RotationRate = bAiming ? FRotator(0.f, AimTurnRate, 0.f) : BaseRotationRate;
	}
}

void UTemporalAimComponent::ApplyCamera()
{
	if (Boom)
	{
		Boom->TargetArmLength = FMath::Lerp(ArmLength, AimArmLength, AimAlpha);
		Boom->SocketOffset = FMath::Lerp(ShoulderOffset, AimShoulderOffset, AimAlpha);
	}
}

void UTemporalAimComponent::ApplyWalkSpeed()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = FMath::Lerp(BaseWalkSpeed, AimWalkSpeed, AimAlpha);
	}
}

AActor* UTemporalAimComponent::FindTarget() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	const UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>();
	if (!PC || !Subsystem)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector Forward = ViewRotation.Vector();

	auto IsSelectable = [Subsystem](const AActor* Actor)
	{
		return Actor && (Actor->FindComponentByClass<UTemporalRecorderComponent>() || Subsystem->IsTimeTarget(Actor));
	};

	// 1) Lo que está justo bajo la mira gana siempre (cajas, plataformas, pedazos: todo lo que bloquea la vista).
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TemporalAimDirect), false, Pawn);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + Forward * MaxDistance, ECC_Visibility, Params)
			&& IsSelectable(Hit.GetActor()))
		{
			return Hit.GetActor();
		}
	}

	// 2) Si no, el más cercano a la mira por ángulo (así entran las balas, que no bloquean la vista).
	//    Los que cubren la mira empatan en 0 y desempata el centro más cercano: una plataforma enorme junto
	//    a ti ya no le gana a la bala que estás apuntando.
	struct FCandidate
	{
		AActor* Actor;
		FVector Center;
		float EdgeAngle;
		float CenterAngle;
	};
	TArray<AActor*> Pool = Subsystem->GetTimeTargets();
	for (UTemporalRecorderComponent* Recorder : Subsystem->GetRecorders())
	{
		Pool.AddUnique(Recorder->GetOwner());
	}

	TArray<FCandidate> Candidates;
	for (AActor* Target : Pool)
	{
		if (!IsValid(Target) || Target == Pawn)
		{
			continue;
		}
		FVector Center, Extent;
		Target->GetActorBounds(false, Center, Extent);
		const FVector ToTarget = Center - ViewLocation;
		const float Distance = ToTarget.Size();
		if (Distance < 1.f || Distance > MaxDistance)
		{
			continue;
		}
		const float CenterAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToTarget / Distance, Forward), -1.f, 1.f)));
		const float Radius = FMath::Min(Extent.Size(), Distance * 0.9f);
		const float EdgeAngle = CenterAngle - FMath::RadiansToDegrees(FMath::Atan(Radius / Distance));
		if (EdgeAngle < MaxAngleDegrees)
		{
			Candidates.Add({ Target, Center, EdgeAngle, CenterAngle });
		}
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return IsBetterAim(A.EdgeAngle, A.CenterAngle, B.EdgeAngle, B.CenterAngle); });

	// El primero que se vea de verdad. Si lo que tapa la vista es de su mismo grupo (otro pedazo del mismo
	// objeto roto), cuenta como visible: así un montón de escombros sí se puede apuntar.
	for (int32 i = 0; i < FMath::Min(Candidates.Num(), 4); ++i)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TemporalAim), false, Pawn);
		Params.AddIgnoredActor(Candidates[i].Actor);
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, Candidates[i].Center, ECC_Visibility, Params))
		{
			return Candidates[i].Actor;
		}
		UTemporalRecorderComponent* Mine = Candidates[i].Actor->FindComponentByClass<UTemporalRecorderComponent>();
		const UTemporalRecorderComponent* Blocker = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UTemporalRecorderComponent>() : nullptr;
		if (Mine && Blocker)
		{
			TSet<UTemporalRecorderComponent*> Group;
			Mine->GatherGroup(Group);
			if (Group.Contains(Blocker))
			{
				return Candidates[i].Actor;
			}
		}
	}
	return nullptr;
}

bool UTemporalAimComponent::IsBetterAim(float EdgeAngleA, float CenterAngleA, float EdgeAngleB, float CenterAngleB)
{
	// Un borde "negativo" solo significa que el objeto cubre la mira: no debe premiar a los grandes o cercanos.
	const float ScoreA = FMath::Max(EdgeAngleA, 0.f);
	const float ScoreB = FMath::Max(EdgeAngleB, 0.f);
	if (!FMath::IsNearlyEqual(ScoreA, ScoreB, 0.01f))
	{
		return ScoreA < ScoreB;
	}
	return CenterAngleA < CenterAngleB;
}

void UTemporalAimComponent::Select(AActor* Target)
{
	if (Target == Hovered.Get() && (Target || Highlighted.Num() == 0))
	{
		return;
	}
	ClearHighlight();
	Hovered = Target;
	if (!Target || !HighlightMaterial)
	{
		return;
	}

	// Con Registro se resalta el grupo entero (los pedazos de un objeto roto, o una bala con lo que rompió).
	TArray<AActor*> Owners;
	if (UTemporalRecorderComponent* Recorder = Target->FindComponentByClass<UTemporalRecorderComponent>())
	{
		TSet<UTemporalRecorderComponent*> Group;
		Recorder->GatherGroup(Group);
		for (UTemporalRecorderComponent* Member : Group)
		{
			Owners.AddUnique(Member->GetOwner());
		}
	}
	else
	{
		Owners.Add(Target);
	}

	for (AActor* Owner : Owners)
	{
		TArray<UMeshComponent*> Meshes;
		Owner->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh->IsVisible())
			{
				Mesh->SetOverlayMaterial(HighlightMaterial);
				Highlighted.Add(Mesh);
			}
		}
	}
}

void UTemporalAimComponent::ClearHighlight()
{
	for (const TWeakObjectPtr<UMeshComponent>& Mesh : Highlighted)
	{
		if (Mesh.IsValid() && Mesh->GetOverlayMaterial() == HighlightMaterial)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	Highlighted.Reset();
	Hovered = nullptr;
}

void UTemporalAimComponent::LockTarget(AActor* Target)
{
	Locked = Target;
	Select(Target);
}

void UTemporalAimComponent::UnlockTarget()
{
	Locked = nullptr;
}

void UTemporalAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Tiempo real: apuntar se siente igual con la Sobremarcha o el tiempo de reflejo.
	AimAlpha = FMath::FInterpTo(AimAlpha, bAiming ? 1.f : 0.f, TemporalFX::RealDelta(), BlendSpeed);
	ApplyCamera();
	ApplyWalkSpeed();

	// La selección sigue a la mira, salvo que esté fija.
	Select(Locked.IsValid() ? Locked.Get() : FindTarget());
}
