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
