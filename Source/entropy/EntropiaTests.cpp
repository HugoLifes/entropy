// ENTROPÍA · Pruebas automáticas (Automation Tests)
// Se corren sin abrir el editor:
//   UnrealEditor-Cmd.exe entropy.uproject -ExecCmds="Automation RunTests Entropia;Quit" -unattended -nullrhi -nosplash -NoSound -log
// Solo prueban lógica que no necesita pantalla (energía, grupos de rebobinado...). Lo visual se prueba en Play.

#include "Misc/AutomationTest.h"
#include "TemporalEnergyComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace EntropiaTestFlags
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaSmokeTest, "Entropia.Smoke", EntropiaTestFlags::Flags)
bool FEntropiaSmokeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("La infraestructura de pruebas corre"), true);
	return true;
}

// ---------------------------------------------------------------------------
// Energía compartida

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaEnergySpendFailsWhenShort, "Entropia.Energy.SpendFailsWhenShort", EntropiaTestFlags::Flags)
bool FEntropiaEnergySpendFailsWhenShort::RunTest(const FString& Parameters)
{
	UTemporalEnergyComponent* Energy = NewObject<UTemporalEnergyComponent>();
	Energy->Energy = 20.f;
	TestFalse(TEXT("Spend(30) con 20 no alcanza"), Energy->Spend(30.f));
	TestEqual(TEXT("Si no alcanza no se gasta nada"), Energy->Energy, 20.f);
	TestTrue(TEXT("Spend(10) con 20 sí alcanza"), Energy->Spend(10.f));
	TestEqual(TEXT("Quedan 10"), Energy->Energy, 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaEnergyDrainStopsAtZero, "Entropia.Energy.DrainStopsAtZero", EntropiaTestFlags::Flags)
bool FEntropiaEnergyDrainStopsAtZero::RunTest(const FString& Parameters)
{
	UTemporalEnergyComponent* Energy = NewObject<UTemporalEnergyComponent>();
	Energy->Energy = 10.f;
	TestFalse(TEXT("Drain(34/s, 0.5 s) con 10 se acaba"), Energy->Drain(34.f, 0.5f));
	TestEqual(TEXT("Se queda en 0, nunca negativa"), Energy->Energy, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaEnergyRegenAfterDelay, "Entropia.Energy.RegenAfterDelay", EntropiaTestFlags::Flags)
bool FEntropiaEnergyRegenAfterDelay::RunTest(const FString& Parameters)
{
	UTemporalEnergyComponent* Energy = NewObject<UTemporalEnergyComponent>();
	Energy->Energy = 50.f;
	Energy->Spend(0.1f);
	const float AfterSpend = Energy->Energy;
	Energy->Advance(0.5f);
	TestEqual(TEXT("Antes de 1 s sin gastar no recarga"), Energy->Energy, AfterSpend);
	Energy->Advance(0.6f);
	TestTrue(TEXT("Después de 1 s sin gastar recarga"), Energy->Energy > AfterSpend);
	for (int32 i = 0; i < 20; ++i)
	{
		Energy->Advance(1.f);
	}
	TestEqual(TEXT("Nunca pasa del máximo"), Energy->Energy, Energy->MaxEnergy);
	return true;
}

#endif
