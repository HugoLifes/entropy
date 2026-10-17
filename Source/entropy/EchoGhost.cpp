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
		// Ya estamos en el frame 0: el siguiente a alcanzar es el 1 (así se mezcla desde el inicio).
		FrameIndex = 1;
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

	// Entre dos frames mezclamos la posición para que el eco se mueva suave aunque cambien los FPS.
	if (FrameIndex > 0 && FrameIndex < Frames.Num() && Frames[FrameIndex].DeltaTime > KINDA_SMALL_NUMBER)
	{
		const float Alpha = FMath::Clamp(Accumulator / Frames[FrameIndex].DeltaTime, 0.f, 1.f);
		FTransform Blended;
		Blended.Blend(Frames[FrameIndex - 1].Transform, Frames[FrameIndex].Transform, Alpha);
		SetActorTransform(Blended, false, nullptr, ETeleportType::TeleportPhysics);

		// También la pose, hueso por hueso: la animación del eco ya no salta de frame en frame.
		const TArray<FTransform>& FromPose = Frames[FrameIndex - 1].Pose;
		const TArray<FTransform>& ToPose = Frames[FrameIndex].Pose;
		const int32 BoneCount = PoseMesh->BoneSpaceTransforms.Num();
		if (BoneCount > 0 && FromPose.Num() == BoneCount && ToPose.Num() == BoneCount)
		{
			for (int32 i = 0; i < BoneCount; ++i)
			{
				FTransform Bone = FromPose[i];
				Bone.BlendWith(ToPose[i], Alpha);
				PoseMesh->BoneSpaceTransforms[i] = Bone;
			}
			PoseMesh->MarkRefreshTransformDirty();
		}
	}

	if (FrameIndex >= Frames.Num())
	{
		// Con un solo frame no hay nada que repetir (y sin esto soltaríamos destellos cada frame).
		if (bLooping && Frames.Num() > 1)
		{
			// Al reiniciar el bucle el eco "salta" al inicio: lo disimulamos con un destello en ambos lados.
			TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.35f, 0.65f, 1.f), 0.5f);
			ApplyFrame(Frames[0]);
			TemporalFX::Burst(this, GetActorLocation(), FLinearColor(0.35f, 0.65f, 1.f), 0.5f);
			FrameIndex = 1;
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
