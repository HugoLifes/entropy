// ENTROPÍA · Escombros y polvo reversibles

#include "TemporalDebrisComponent.h"
#include "TemporalFX.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace EntropiaDebris
{
	constexpr float Gravity = -980.f;
	/** Arrastre del polvo (1/s): más alto = se frena antes. */
	constexpr float DustDrag = 2.2f;

	float SmoothStep01(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}
}

UTemporalDebrisComponent::UTemporalDebrisComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Solo tickea mientras hay escombros que mover.
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

FVector UTemporalDebrisComponent::EvaluateChip(const FDebrisParticle& P, float InT, float FloorZ)
{
	using namespace EntropiaDebris;
	const float RestZ = FloorZ + P.Size * 0.5f;
	// Momento en que toca el piso: Origin.Z + Vz·t + ½·g·t² = RestZ  (raíz positiva).
	// Si nunca llega a esa altura (nació por debajo del piso), se queda en el piso desde el inicio.
	float LandTime = 0.f;
	const float A = 0.5f * Gravity;
	const float B = P.Velocity.Z;
	const float C = P.Origin.Z - RestZ;
	const float Discriminant = B * B - 4.f * A * C;
	if (Discriminant >= 0.f)
	{
		LandTime = FMath::Max(0.f, (-B - FMath::Sqrt(Discriminant)) / (2.f * A));
	}

	const float Time = FMath::Max(0.f, FMath::Min(InT, LandTime));
	FVector Position = P.Origin + P.Velocity * Time;
	Position.Z += 0.5f * Gravity * Time * Time;
	if (InT >= LandTime)
	{
		Position.Z = RestZ;
	}
	return Position;
}

float UTemporalDebrisComponent::DustScale(float InT, float InLife)
{
	using namespace EntropiaDebris;
	if (InLife <= 0.f)
	{
		return 0.f;
	}
	const float A = InT / InLife;
	if (A <= 0.f || A >= 1.f)
	{
		return 0.f;
	}
	return A < 0.25f ? SmoothStep01(A / 0.25f) : 1.f - SmoothStep01((A - 0.25f) / 0.75f);
}

FVector UTemporalDebrisComponent::EvaluateDust(const FDebrisParticle& P, float InT)
{
	using namespace EntropiaDebris;
	// x(t) = x0 + v · (1 − e^(−k·t)) / k : sale rápido y se frena, como polvo en el aire.
	const float Spread = (1.f - FMath::Exp(-DustDrag * FMath::Max(0.f, InT))) / DustDrag;
	return P.Origin + P.Velocity * Spread;
}

void UTemporalDebrisComponent::EnsureComponents()
{
	if (ChipMesh && DustMesh)
	{
		return;
	}
	AActor* Owner = GetOwner();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	auto MakeISM = [Owner](UStaticMesh* MeshAsset, const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
		ISM->SetStaticMesh(MeshAsset);
		ISM->SetupAttachment(Owner->GetRootComponent());
		// En espacio de mundo: el objeto puede estar escalado de forma no uniforme (el muro de 40 × 500 × 300).
		ISM->SetUsingAbsoluteLocation(true);
		ISM->SetUsingAbsoluteRotation(true);
		ISM->SetUsingAbsoluteScale(true);
		ISM->SetWorldTransform(FTransform::Identity);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetGenerateOverlapEvents(false);
		ISM->SetCastShadow(false);
		ISM->RegisterComponent();
		return ISM;
	};

	ChipMesh = MakeISM(Cube, TEXT("DebrisChips"));
	DustMesh = MakeISM(Sphere, TEXT("DebrisDust"));
	TemporalFX::ApplyToAllSlots(DustMesh, TemporalFX::MakeTranslucent(this, DustColor));
}

void UTemporalDebrisComponent::Burst(const FVector& ImpactPoint, const FVector& Direction, const FLinearColor& ChipColor, float FloorZ)
{
	EnsureComponents();
	if (!ChipMaterial)
	{
		ChipMaterial = TemporalFX::MakeSolid(this, ChipColor);
		TemporalFX::ApplyToAllSlots(ChipMesh, ChipMaterial);
	}
	else
	{
		ChipMaterial->SetVectorParameterValue(TEXT("Color"), ChipColor);
	}

	const FVector Push = Direction.GetSafeNormal();
	Chips.Reset();
	for (int32 i = 0; i < NumChips; ++i)
	{
		FDebrisParticle Chip;
		Chip.Origin = ImpactPoint + FMath::VRand() * FMath::FRandRange(0.f, 15.f);
		// Hacia afuera y hacia arriba, con algo del empuje del golpe.
		FVector Out = FMath::VRand();
		Out.Z = FMath::Abs(Out.Z) * 0.8f + 0.2f;
		Chip.Velocity = (Out + Push * 0.5f).GetSafeNormal() * FMath::FRandRange(250.f, 650.f);
		Chip.Size = FMath::FRandRange(6.f, 12.f);
		Chip.SpinDeg = FMath::FRandRange(-720.f, 720.f);
		Chip.SpinAxis = FMath::VRand();
		Chips.Add(Chip);
	}

	Dust.Reset();
	for (int32 i = 0; i < NumDust; ++i)
	{
		FDebrisParticle Puff;
		Puff.Origin = ImpactPoint + FMath::VRand() * 10.f;
		FVector Out = FMath::VRand();
		Out.Z = FMath::Abs(Out.Z) * 0.5f;
		Puff.Velocity = (Out + Push * 0.3f).GetSafeNormal() * FMath::FRandRange(80.f, 220.f);
		Puff.Size = FMath::FRandRange(50.f, 110.f);
		Dust.Add(Puff);
	}

	// Una instancia por partícula, creada una vez; luego solo se mueven.
	ChipMesh->ClearInstances();
	DustMesh->ClearInstances();
	for (int32 i = 0; i < Chips.Num(); ++i)
	{
		ChipMesh->AddInstance(FTransform(ImpactPoint), /*bWorldSpace=*/true);
	}
	for (int32 i = 0; i < Dust.Num(); ++i)
	{
		DustMesh->AddInstance(FTransform(ImpactPoint), /*bWorldSpace=*/true);
	}

	FloorHeight = FloorZ;
	T = 0.f;
	bActive = true;
	bRewinding = false;
	ChipMesh->SetVisibility(true);
	DustMesh->SetVisibility(true);
	UpdateInstances();
	SetComponentTickEnabled(true);
}

void UTemporalDebrisComponent::Hide()
{
	bActive = false;
	bRewinding = false;
	T = 0.f;
	if (ChipMesh)
	{
		ChipMesh->ClearInstances();
		ChipMesh->SetVisibility(false);
	}
	if (DustMesh)
	{
		DustMesh->ClearInstances();
		DustMesh->SetVisibility(false);
	}
	Chips.Reset();
	Dust.Reset();
	SetComponentTickEnabled(false);
}

void UTemporalDebrisComponent::SetRewinding(bool bInRewinding, float InRewindSpeed)
{
	bRewinding = bInRewinding;
	RewindSpeed = FMath::Max(0.1f, InRewindSpeed);
	if (bActive && bRewinding)
	{
		SetComponentTickEnabled(true);
	}
}

void UTemporalDebrisComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float RealDelta = TemporalFX::RealDelta();
	if (bRewinding)
	{
		T = FMath::Max(0.f, T - RealDelta * RewindSpeed);
	}
	else
	{
		T = FMath::Min(Life, T + RealDelta);
	}
	UpdateInstances();

	// Ya desapareció todo y nadie rebobina: dejamos de tickear (pero recordamos T por si luego rebobinan).
	if (!bRewinding && T >= Life)
	{
		SetComponentTickEnabled(false);
	}
}

void UTemporalDebrisComponent::UpdateInstances()
{
	if (!ChipMesh || !DustMesh)
	{
		return;
	}

	// Astillas: aparecen en los primeros 0.06 s y se encogen en el último 30 % de su vida.
	const float ChipIn = EntropiaDebris::SmoothStep01(T / 0.06f);
	const float ChipOut = 1.f - EntropiaDebris::SmoothStep01((T - Life * 0.7f) / (Life * 0.3f));
	for (int32 i = 0; i < Chips.Num(); ++i)
	{
		const FDebrisParticle& Chip = Chips[i];
		const FVector Location = EvaluateChip(Chip, T, FloorHeight);
		const bool bLanded = FMath::IsNearlyEqual(Location.Z, FloorHeight + Chip.Size * 0.5f, 0.5f);
		const float Spin = bLanded ? 0.f : Chip.SpinDeg * T;
		const FQuat Rotation(Chip.SpinAxis, FMath::DegreesToRadians(Spin));
		const float Scale = Chip.Size / 100.f * ChipIn * ChipOut;
		ChipMesh->UpdateInstanceTransform(i, FTransform(Rotation, Location, FVector(FMath::Max(Scale, 0.0001f))), true, false, true);
	}
	for (int32 i = 0; i < Dust.Num(); ++i)
	{
		const FDebrisParticle& Puff = Dust[i];
		const float Scale = Puff.Size / 100.f * DustScale(T, Life);
		DustMesh->UpdateInstanceTransform(i, FTransform(FQuat::Identity, EvaluateDust(Puff, T), FVector(FMath::Max(Scale, 0.0001f))), true, false, true);
	}
	ChipMesh->MarkRenderStateDirty();
	DustMesh->MarkRenderStateDirty();
}
