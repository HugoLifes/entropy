// ENTROPÍA · Registro Temporal

#include "TemporalRecorderComponent.h"
#include "TemporalFX.h"
#include "TemporalSubsystem.h"
#include "Misc/App.h"
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

	if (UTemporalSubsystem* Subsystem = GetWorld()->GetSubsystem<UTemporalSubsystem>())
	{
		Subsystem->Register(this);
	}
}

void UTemporalRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTemporalSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UTemporalSubsystem>() : nullptr)
	{
		Subsystem->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UTemporalRecorderComponent::LinkWith(UTemporalRecorderComponent* Other)
{
	if (!Other || Other == this)
	{
		return;
	}
	Links.AddUnique(Other);
	Other->Links.AddUnique(this);
}

void UTemporalRecorderComponent::GatherGroup(TSet<UTemporalRecorderComponent*>& OutGroup)
{
	// Recorrido a lo ancho: así un enlace A-B-C también trae a C al apuntar a A.
	TArray<UTemporalRecorderComponent*> Pending = { this };
	while (Pending.Num() > 0)
	{
		UTemporalRecorderComponent* Current = Pending.Pop();
		if (!IsValid(Current) || OutGroup.Contains(Current))
		{
			continue;
		}
		OutGroup.Add(Current);
		for (const TWeakObjectPtr<UTemporalRecorderComponent>& Link : Current->Links)
		{
			if (Link.IsValid())
			{
				Pending.Add(Link.Get());
			}
		}
	}
}

bool UTemporalRecorderComponent::GetRewindSegment(const FTemporalFrame*& OutFrom, const FTemporalFrame*& OutTo, float& OutAlpha) const
{
	if (!IsRewinding() || NumFrames() == 0)
	{
		return false;
	}
	OutFrom = &LastRewoundFrame;
	OutTo = &Frames.Last();
	OutAlpha = RewindAlpha;
	return true;
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
		// DeltaTime ya viene multiplicado por la dilatación (0 dentro de una burbuja; 0.25 con la Sobremarcha).
		// Usamos el mayor entre ese y el tiempo real: ni las burbujas ni la Sobremarcha frenan tu rebobinado.
		RewindStep(bRewindIgnoresTimeDilation ? FMath::Max(DeltaTime, TemporalFX::RealDelta()) : DeltaTime);
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
	// El frame más reciente es "donde estamos ahora": de aquí parte el primer tramo hacia atrás.
	LastRewoundFrame = Frames.Pop();
	RecordedSeconds = FMath::Max(0.f, RecordedSeconds - LastRewoundFrame.DeltaTime);

	// Mientras rebobinamos, la física no debe pelear con nosotros.
	bWasSimulatingPhysics = PhysicsRoot && PhysicsRoot->IsSimulatingPhysics();
	if (bWasSimulatingPhysics)
	{
		PhysicsRoot->SetSimulatePhysics(false);
	}
}

void UTemporalRecorderComponent::RewindStep(float DeltaTime)
{
	// Si alguien le regresó la física a medio rebobinado (p. ej. terminó una burbuja de estasis),
	// la volvemos a apagar y la devolvemos al soltar, para que no pelee con nosotros.
	if (PhysicsRoot && PhysicsRoot->IsSimulatingPhysics())
	{
		PhysicsRoot->SetSimulatePhysics(false);
		bWasSimulatingPhysics = true;
	}

	RewindAccumulator += DeltaTime * RewindSpeed;

	// Ir de LastRewoundFrame al frame anterior tarda lo mismo que tardó grabarse: LastRewoundFrame.DeltaTime.
	while (NumFrames() > 0 && RewindAccumulator >= LastRewoundFrame.DeltaTime)
	{
		RewindAccumulator -= LastRewoundFrame.DeltaTime;
		LastRewoundFrame = Frames.Pop();
		RecordedSeconds = FMath::Max(0.f, RecordedSeconds - LastRewoundFrame.DeltaTime);
	}

	// Mezclamos entre el frame actual y el anterior: así no se ve a saltos aunque bajen los FPS
	// o RewindSpeed no coincida con el ritmo de grabación.
	FTransform Target = LastRewoundFrame.Transform;
	RewindAlpha = 0.f;
	if (NumFrames() > 0 && LastRewoundFrame.DeltaTime > KINDA_SMALL_NUMBER)
	{
		RewindAlpha = FMath::Clamp(RewindAccumulator / LastRewoundFrame.DeltaTime, 0.f, 1.f);
		Target.Blend(LastRewoundFrame.Transform, Frames.Last().Transform, RewindAlpha);
	}
	GetOwner()->SetActorTransform(Target, false, nullptr, ETeleportType::TeleportPhysics);

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
		// Que no se quede "dormido" donde lo dejó el rebobinado (si no, puede flotar hasta que algo lo toque).
		PhysicsRoot->WakeAllRigidBodies();
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
	// Si estábamos rebobinando, terminamos limpio (física de vuelta, aviso a quien escuche).
	if (IsRewinding())
	{
		StopRewind();
	}
	Frames.Reset();
	FirstFrame = 0;
	RecordedSeconds = 0.f;
	RewindAccumulator = 0.f;
	// También olvidamos el último frame rebobinado (su velocidad la usa el Paso Inverso al terminar).
	LastRewoundFrame = FTemporalFrame();
}
