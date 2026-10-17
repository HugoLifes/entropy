// ENTROPÍA · Lista de Registros Temporales

#include "TemporalSubsystem.h"
#include "TemporalRecorderComponent.h"

void UTemporalSubsystem::Register(UTemporalRecorderComponent* Recorder)
{
	if (Recorder)
	{
		Recorders.AddUnique(Recorder);
	}
}

void UTemporalSubsystem::Unregister(UTemporalRecorderComponent* Recorder)
{
	Recorders.RemoveAll([Recorder](const TWeakObjectPtr<UTemporalRecorderComponent>& R) { return !R.IsValid() || R.Get() == Recorder; });
}

TArray<UTemporalRecorderComponent*> UTemporalSubsystem::GetRecorders() const
{
	TArray<UTemporalRecorderComponent*> Result;
	Result.Reserve(Recorders.Num());
	for (const TWeakObjectPtr<UTemporalRecorderComponent>& Recorder : Recorders)
	{
		if (Recorder.IsValid())
		{
			Result.Add(Recorder.Get());
		}
	}
	return Result;
}

void UTemporalSubsystem::RegisterTimeTarget(AActor* Actor)
{
	if (Actor)
	{
		TimeTargets.AddUnique(Actor);
	}
}

void UTemporalSubsystem::UnregisterTimeTarget(AActor* Actor)
{
	TimeTargets.RemoveAll([Actor](const TWeakObjectPtr<AActor>& T) { return !T.IsValid() || T.Get() == Actor; });
}

TArray<AActor*> UTemporalSubsystem::GetTimeTargets() const
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& Target : TimeTargets)
	{
		if (Target.IsValid())
		{
			Result.Add(Target.Get());
		}
	}
	return Result;
}

bool UTemporalSubsystem::IsTimeTarget(const AActor* Actor) const
{
	return Actor && TimeTargets.ContainsByPredicate([Actor](const TWeakObjectPtr<AActor>& T) { return T.Get() == Actor; });
}
