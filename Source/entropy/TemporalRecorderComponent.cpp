// ENTROPÍA · Registro Temporal

#include "TemporalRecorderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UTemporalRecorderComponent::UTemporalRecorderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Grabamos después de la física para guardar la posición final de cada frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UTemporalRecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	PhysicsRoot = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		PoseSource = Character->GetMesh();
	}
	else
	{
		PoseSource = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}
}

void UTemporalRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (State)
	{
	case ETemporalState::Recording:
		RecordFrame(DeltaTime);
		break;
	case ETemporalState::Rewinding:
		RewindStep(DeltaTime);
		break;
	default:
		break;
	}
}

void UTemporalRecorderComponent::RecordFrame(float DeltaTime)
{
	if (DeltaTime <= 0.f)
	{
		return;
	}

	FTemporalFrame Frame;
	Frame.Transform = GetOwner()->GetActorTransform();
	Frame.DeltaTime = DeltaTime;

	if (PhysicsRoot && PhysicsRoot->IsSimulatingPhysics())
	{
		Frame.LinearVelocity = PhysicsRoot->GetPhysicsLinearVelocity();
		Frame.AngularVelocity = PhysicsRoot->GetPhysicsAngularVelocityInDegrees();
	}
	else
	{
		Frame.LinearVelocity = GetOwner()->GetVelocity();
	}

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Frame.bFalling = Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
	}

	if (bRecordPose && PoseSource && PoseSource->GetSkeletalMeshAsset())
	{
		Frame.Pose = PoseSource->GetBoneSpaceTransforms();
	}

	Frames.Add(MoveTemp(Frame));
	RecordedSeconds += DeltaTime;

	// Olvidamos lo más viejo cuando pasamos del límite. En vez de recorrer todo el arreglo
	// cada frame (RemoveAt(0)), solo avanzamos FirstFrame y compactamos de vez en cuando.
	while (RecordedSeconds > MaxRecordSeconds && NumFrames() > 1)
	{
		RecordedSeconds -= Frames[FirstFrame].DeltaTime;
		Frames[FirstFrame].Pose.Empty();
		++FirstFrame;
	}
	if (FirstFrame > 256 && FirstFrame * 2 > Frames.Num())
	{
		Frames.RemoveAt(0, FirstFrame, EAllowShrinking::No);
		FirstFrame = 0;
	}
}

TArray<FTemporalFrame> UTemporalRecorderComponent::GetRecording() const
{
	return TArray<FTemporalFrame>(Frames.GetData() + FirstFrame, NumFrames());
}

void UTemporalRecorderComponent::StartRewind()
{
	if (State == ETemporalState::Rewinding || NumFrames() == 0)
	{
		return;
	}

	State = ETemporalState::Rewinding;
	RewindAccumulator = 0.f;
	LastRewoundFrame = Frames.Last();

	// Mientras rebobinamos, la física no debe pelear con nosotros.
	bWasSimulatingPhysics = PhysicsRoot && PhysicsRoot->IsSimulatingPhysics();
	if (bWasSimulatingPhysics)
	{
		PhysicsRoot->SetSimulatePhysics(false);
	}
}

void UTemporalRecorderComponent::RewindStep(float DeltaTime)
{
	RewindAccumulator += DeltaTime * RewindSpeed;

	// Sacamos frames del final al mismo ritmo con que se grabaron.
	while (NumFrames() > 0 && RewindAccumulator >= Frames.Last().DeltaTime)
	{
		RewindAccumulator -= Frames.Last().DeltaTime;
		LastRewoundFrame = Frames.Pop();
		RecordedSeconds = FMath::Max(0.f, RecordedSeconds - LastRewoundFrame.DeltaTime);
		GetOwner()->SetActorTransform(LastRewoundFrame.Transform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (NumFrames() == 0)
	{
		Frames.Reset();
		FirstFrame = 0;
		StopRewind();
	}
}

void UTemporalRecorderComponent::StopRewind()
{
	if (State != ETemporalState::Rewinding)
	{
		return;
	}

	State = ETemporalState::Recording;
	RewindAccumulator = 0.f;

	if (bWasSimulatingPhysics && PhysicsRoot)
	{
		PhysicsRoot->SetSimulatePhysics(true);
		const float Direction = bInvertMomentumOnRelease ? -1.f : 1.f;
		PhysicsRoot->SetPhysicsLinearVelocity(LastRewoundFrame.LinearVelocity * Direction);
		PhysicsRoot->SetPhysicsAngularVelocityInDegrees(LastRewoundFrame.AngularVelocity * Direction);
	}
	bWasSimulatingPhysics = false;

	OnRewindFinished.Broadcast();
}

void UTemporalRecorderComponent::SetPaused(bool bPaused)
{
	if (State == ETemporalState::Rewinding)
	{
		StopRewind();
	}
	State = bPaused ? ETemporalState::Paused : ETemporalState::Recording;
}

void UTemporalRecorderComponent::ClearRecording()
{
	Frames.Reset();
	FirstFrame = 0;
	RecordedSeconds = 0.f;
	RewindAccumulator = 0.f;
}
