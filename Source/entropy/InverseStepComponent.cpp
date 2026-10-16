// ENTROPÍA · Paso Inverso

#include "InverseStepComponent.h"
#include "TemporalRecorderComponent.h"
#include "TemporalFX.h"

#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

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
	Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (Camera)
	{
		BaseFov = Camera->FieldOfView;
	}
	Energy = MaxEnergy;

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

void UInverseStepComponent::BeginInverseStep()
{
	if (bActive || !Recorder || Energy < MinEnergyToStart || Recorder->GetRecordedSeconds() < 0.1f)
	{
		return;
	}

	bActive = true;
	TimeSinceUse = 0.f;

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
}

void UInverseStepComponent::EndInverseStep()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	TimeSinceUse = 0.f;

	if (Recorder && Recorder->IsRewinding())
	{
		Recorder->StopRewind();
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->StopMovementImmediately();
		// Falling resuelve solo: si estás en el piso pasa a Walking en el siguiente frame.
		Movement->SetMovementMode(MOVE_Falling);
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
		Energy = FMath::Max(0.f, Energy - DrainPerSecond * DeltaTime);
		// Se acabó la energía o la grabación: terminamos solos.
		if (Energy <= 0.f || !Recorder || !Recorder->IsRewinding())
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
				SpawnAfterimage();
			}
		}
	}
	else
	{
		TimeSinceUse += DeltaTime;
		if (TimeSinceUse >= RegenDelay)
		{
			Energy = FMath::Min(MaxEnergy, Energy + RegenPerSecond * DeltaTime);
		}
	}

	// El efecto de cámara entra rápido y sale suave.
	const float Target = bActive ? 1.f : 0.f;
	EffectAlpha = FMath::FInterpTo(EffectAlpha, Target, DeltaTime, bActive ? 12.f : 5.f);
	ApplyCameraEffect(EffectAlpha);

	if (bShowDebugHud && GEngine)
	{
		const int32 Filled = FMath::RoundToInt(Energy / MaxEnergy * 10.f);
		FString Bar;
		for (int32 i = 0; i < 10; ++i)
		{
			Bar += (i < Filled) ? TEXT("#") : TEXT("-");
		}
		const FColor Color = bActive ? FColor(120, 200, 255) : (Energy < MinEnergyToStart ? FColor::Red : FColor::White);
		GEngine->AddOnScreenDebugMessage(2000, 0.2f, Color, FString::Printf(TEXT("PASO INVERSO (F)  [%s]  %d%%"), *Bar, FMath::RoundToInt(Energy / MaxEnergy * 100.f)));
	}
}

void UInverseStepComponent::ApplyCameraEffect(float Alpha)
{
	if (!Camera)
	{
		return;
	}

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.25f, 0.25f, 0.25f, 1.f);
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(0.75f, 0.9f, 1.25f, 1.f);
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = 4.f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.9f;

	Camera->PostProcessBlendWeight = Alpha;
	Camera->SetFieldOfView(BaseFov + FovKick * Alpha);
}

void UInverseStepComponent::ApplyReversePose()
{
	if (!PoseProxy || !Recorder)
	{
		return;
	}

	const TArray<FTransform>& Pose = Recorder->GetLastRewoundFrame().Pose;
	if (Pose.Num() > 0 && Pose.Num() == PoseProxy->BoneSpaceTransforms.Num())
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

void UInverseStepComponent::SpawnAfterimage()
{
	if (!BodyMesh || !BodyMesh->GetSkeletalMeshAsset() || !Recorder)
	{
		return;
	}

	// La copia sale de donde está la marioneta (o el cuerpo, si no hay pose grabada).
	const USceneComponent* Source = (PoseProxy && PoseProxy->IsVisible()) ? static_cast<USceneComponent*>(PoseProxy) : static_cast<USceneComponent*>(BodyMesh);
	const FTransform MeshTransform = Source->GetComponentTransform();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATemporalAfterimage* Ghost = GetWorld()->SpawnActor<ATemporalAfterimage>(ATemporalAfterimage::StaticClass(), MeshTransform, Params))
	{
		Ghost->Init(BodyMesh->GetSkeletalMeshAsset(), MeshTransform, Recorder->GetLastRewoundFrame().Pose, AfterimageColor, AfterimageLife);
	}
}
