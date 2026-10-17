// ENTROPÍA · Paso Inverso

#include "InverseStepComponent.h"
#include "TemporalEnergyComponent.h"
#include "TemporalFX.h"
#include "TemporalRecorderComponent.h"
#include "TemporalTrailComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Misc/App.h"

UInverseStepComponent::UInverseStepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Después de la física: el Registro Temporal ya movió al personaje este frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UInverseStepComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	Recorder = Owner->FindComponentByClass<UTemporalRecorderComponent>();
	if (Recorder)
	{
		// Los dos van en TG_PostPhysics: así garantizamos que el Registro mueva al personaje primero
		// y la pose al revés no se quede un frame atrasada.
		AddTickPrerequisiteComponent(Recorder);
	}
	Energy = TemporalFX::FindOrAddComponent<UTemporalEnergyComponent>(Owner, TEXT("TemporalEnergy"));
	Trail = TemporalFX::FindOrAddComponent<UTemporalTrailComponent>(Owner, TEXT("TemporalTrail"));

	Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (Camera)
	{
		BaseFov = Camera->FieldOfView;
	}

	// Mismo look de siempre (estilo A): mundo deslavado, tinte azul, bordes de color y viñeta.
	PostProcess = NewObject<UPostProcessComponent>(Owner, TEXT("InversePostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 1.f;
	PostProcess->BlendWeight = 0.f;
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.25f, 0.25f, 0.25f, 1.f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(0.75f, 0.9f, 1.25f, 1.f);
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = 4.f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.9f;
	PostProcess->RegisterComponent();

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		BodyMesh = Character->GetMesh();
	}

	// Marioneta con la misma malla: se muestra solo al rebobinar para reproducir la pose al revés.
	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		PoseProxy = NewObject<UPoseableMeshComponent>(Owner, TEXT("InversePoseProxy"));
		PoseProxy->SetSkinnedAssetAndUpdate(BodyMesh->GetSkeletalMeshAsset());
		PoseProxy->SetupAttachment(BodyMesh->GetAttachParent());
		PoseProxy->SetRelativeTransform(BodyMesh->GetRelativeTransform());
		PoseProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PoseProxy->SetVisibility(false);
		PoseProxy->RegisterComponent();
	}
}

bool UInverseStepComponent::BeginInverseStep()
{
	if (bActive)
	{
		return true;
	}
	if (!Recorder || Recorder->GetRecordedSeconds() < 0.1f)
	{
		return false;
	}
	if (!Energy || !Energy->CanAfford(MinToStart))
	{
		if (Energy)
		{
			Energy->Deny();
		}
		return false;
	}

	bActive = true;
	Energy->SetActivePower(TEXT("Inverso"));

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->DisableMovement();
		if (AController* Controller = Character->GetController())
		{
			Controller->SetIgnoreMoveInput(true);
		}
	}

	Recorder->StartRewind();
	SetPoseProxyVisible(true);
	AfterimageTimer = 0.f;
	TemporalFX::Burst(this, GetOwner()->GetActorLocation(), FLinearColor(0.3f, 0.6f, 1.f), 0.7f);
	return true;
}

void UInverseStepComponent::EndInverseStep()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	if (Energy)
	{
		Energy->SetActivePower(NAME_None);
	}

	if (Recorder && Recorder->IsRewinding())
	{
		Recorder->StopRewind();
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		// Retomas el pasado tal como era: con la velocidad y el estado (suelo/aire) de ese instante.
		const FTemporalFrame& Past = Recorder ? Recorder->GetLastRewoundFrame() : FTemporalFrame();
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(Past.bFalling ? MOVE_Falling : MOVE_Walking);
		Movement->Velocity = Past.LinearVelocity * ExitMomentumScale;
		if (AController* Controller = Character->GetController())
		{
			Controller->ResetIgnoreMoveInput();
		}
	}

	SetPoseProxyVisible(false);
	TemporalFX::Burst(this, GetOwner()->GetActorLocation(), FLinearColor(0.8f, 0.9f, 1.f), 0.5f);
}

void UInverseStepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bActive)
	{
		// Se acabó la energía o la grabación: terminamos solos.
		const bool bHasEnergy = Energy && Energy->Drain(CostPerSecond, FApp::GetDeltaTime());
		if (!bHasEnergy || !Recorder || !Recorder->IsRewinding())
		{
			EndInverseStep();
		}
		else
		{
			ApplyReversePose();
			AfterimageTimer += DeltaTime;
			if (AfterimageInterval > 0.f && AfterimageTimer >= AfterimageInterval)
			{
				AfterimageTimer = 0.f;
				EmitAfterimage();
			}
		}
	}

	// El efecto de cámara entra rápido y sale suave.
	const float Target = bActive ? 1.f : 0.f;
	EffectAlpha = FMath::FInterpTo(EffectAlpha, Target, DeltaTime, bActive ? 12.f : 5.f);
	ApplyCameraEffect(EffectAlpha);

	if (bShowDebugHud && GEngine && Energy)
	{
		GEngine->AddOnScreenDebugMessage(2000, 0.2f, FColor::White,
			FString::Printf(TEXT("ENERGIA %d%%"), FMath::RoundToInt(Energy->GetFraction() * 100.f)));
	}
}

void UInverseStepComponent::ApplyCameraEffect(float Alpha)
{
	if (PostProcess)
	{
		PostProcess->BlendWeight = Alpha;
	}
	if (Camera)
	{
		Camera->SetFieldOfView(BaseFov + FovKick * Alpha);
	}
}

void UInverseStepComponent::ApplyReversePose()
{
	if (!PoseProxy || !Recorder)
	{
		return;
	}

	const int32 BoneCount = PoseProxy->BoneSpaceTransforms.Num();

	// Mezclamos hueso por hueso entre el frame actual y el anterior: la animación al revés ya no salta.
	const FTemporalFrame* From = nullptr;
	const FTemporalFrame* To = nullptr;
	float Alpha = 0.f;
	if (Recorder->GetRewindSegment(From, To, Alpha) && From->Pose.Num() == BoneCount && To->Pose.Num() == BoneCount && BoneCount > 0)
	{
		for (int32 i = 0; i < BoneCount; ++i)
		{
			FTransform Bone = From->Pose[i];
			Bone.BlendWith(To->Pose[i], Alpha);
			PoseProxy->BoneSpaceTransforms[i] = Bone;
		}
		PoseProxy->MarkRefreshTransformDirty();
		return;
	}

	const TArray<FTransform>& Pose = Recorder->GetLastRewoundFrame().Pose;
	if (Pose.Num() > 0 && Pose.Num() == BoneCount)
	{
		PoseProxy->BoneSpaceTransforms = Pose;
		PoseProxy->MarkRefreshTransformDirty();
	}
}

void UInverseStepComponent::SetPoseProxyVisible(bool bVisible)
{
	// Solo cambiamos de cuerpo si hay pose grabada; si no, se ve el personaje normal deslizándose.
	const bool bHasPose = Recorder && Recorder->bRecordPose && PoseProxy;
	if (!bHasPose || !BodyMesh)
	{
		return;
	}

	if (bVisible)
	{
		ApplyReversePose();
	}
	PoseProxy->SetVisibility(bVisible);
	BodyMesh->SetVisibility(!bVisible, true);
}

void UInverseStepComponent::EmitAfterimage()
{
	if (!Trail || !BodyMesh || !BodyMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	// La copia sale de la marioneta (con la pose ya mezclada) o, si no hay pose grabada, del cuerpo.
	const bool bFromProxy = PoseProxy && PoseProxy->IsVisible();
	const FTransform MeshTransform = bFromProxy ? PoseProxy->GetComponentTransform() : BodyMesh->GetComponentTransform();
	const TArray<FTransform>& Pose = bFromProxy ? PoseProxy->BoneSpaceTransforms : Recorder->GetLastRewoundFrame().Pose;
	Trail->Emit(BodyMesh->GetSkeletalMeshAsset(), Pose, MeshTransform, AfterimageColor);
}
