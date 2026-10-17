// ENTROPÍA · Pruebas automáticas (Automation Tests)
// Se corren sin abrir el editor:
//   UnrealEditor-Cmd.exe entropy.uproject -ExecCmds="Automation RunTests Entropia;Quit" -unattended -nullrhi -nosplash -NoSound -log
// Solo prueban lógica que no necesita pantalla (energía, grupos de rebobinado...). Lo visual se prueba en Play.

#include "Misc/AutomationTest.h"

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

#endif
