// ENTROPÍA · Eco

#include "EchoGhost.h"
#include "TemporalFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

AEchoGhost::AEchoGhost()
{
	PrimaryActorTick.bCanEverTick = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(42.f, 96.f);
	// Solo traslape: no bloquea al jugador, pero sí activa placas y gatillos.
	Capsule->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Capsule->SetGenerateOverlapEvents(true);
	SetRootComponent(Capsule);

	PoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseMesh"));
	PoseMesh->SetupAttachment(Capsule);
	PoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseMesh->SetCastShadow(false);
}

void AEchoGhost::SetupFromCharacterMesh(USkeletalMeshComponent* SourceMesh, float CapsuleRadius, float CapsuleHalfHeight)
{
	Capsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	if (SourceMesh && SourceMesh->GetSkeletalMeshAsset())
	{
		PoseMesh->SetSkinnedAssetAndUpdate(SourceMesh->GetSkeletalMeshAsset());
		PoseMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
		// Fantasma azul translúcido (el material vive en el componente, no lo borra el recolector).
		TemporalFX::ApplyToAllSlots(PoseMesh, TemporalFX::MakeTranslucent(this, FLinearColor(0.35f, 0.65f, 1.f, 0.45f)));
	}
}

void AEchoGhost::StartReplay(const TArray<FTemporalFrame>& InFrames, bool bLoop)
{
	Frames = InFrames;
	bLooping = bLoop;
	FrameIndex = 0;
	Accumulator = 0.f;
	bPlaying = Frames.Num() > 0;

	if (bPlaying)
	{
		ApplyFrame(Frames[0]);
		TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.35f, 0.65f, 1.f), 0.8f);
	}
}

void AEchoGhost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bPlaying)
	{
		return;
	}

	Accumulator += DeltaTime;
	int32 LastApplied = INDEX_NONE;
	while (FrameIndex < Frames.Num() && Accumulator >= Frames[FrameIndex].DeltaTime)
	{
		Accumulator -= Frames[FrameIndex].DeltaTime;
		LastApplied = FrameIndex;
		++FrameIndex;
	}
	// Solo aplicamos el último frame alcanzado (más barato que aplicar todos).
	if (LastApplied != INDEX_NONE)
	{
		ApplyFrame(Frames[LastApplied]);
	}

	if (FrameIndex >= Frames.Num())
	{
		if (bLooping)
		{
			FrameIndex = 0;
			Accumulator = 0.f;
			return;
		}

		bPlaying = false;
		OnEchoFinished.Broadcast();
		if (bDestroyWhenFinished)
		{
			Destroy();
		}
	}
}

void AEchoGhost::ApplyFrame(const FTemporalFrame& Frame)
{
	// Teletransporte (sin barrido): las placas detectan el traslape igual.
	SetActorTransform(Frame.Transform, false, nullptr, ETeleportType::TeleportPhysics);

	if (Frame.Pose.Num() > 0 && Frame.Pose.Num() == PoseMesh->BoneSpaceTransforms.Num())
	{
		PoseMesh->BoneSpaceTransforms = Frame.Pose;
		PoseMesh->MarkRefreshTransformDirty();
	}
}
