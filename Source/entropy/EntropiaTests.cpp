// ENTROPÍA · Pruebas automáticas (Automation Tests)
// Se corren sin abrir el editor:
//   UnrealEditor-Cmd.exe entropy.uproject -ExecCmds="Automation RunTests Entropia;Quit" -unattended -nullrhi -nosplash -NoSound -log
// Solo prueban lógica que no necesita pantalla (energía, grupos de rebobinado...). Lo visual se prueba en Play.

#include "Misc/AutomationTest.h"
#include "TemporalEnergyComponent.h"
#include "TemporalRecorderComponent.h"
#include "TemporalAimComponent.h"
#include "TemporalFX.h"
#include "Misc/App.h"
#include "TemporalFracture.h"
#include "TemporalDebrisComponent.h"
#include "TemporalFXSubsystem.h"
#include "TemporalTrailComponent.h"
#include "TemporalDilation.h"
#include "TemporalDashComponent.h"
#include "TemporalHealthComponent.h"
#include "TemporalCombat.h"
#include "TakedownComponent.h"

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

// ---------------------------------------------------------------------------
// Grupos de rebobinado (Invertir objeto)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaGroupTransitive, "Entropia.Group.Transitive", EntropiaTestFlags::Flags)
bool FEntropiaGroupTransitive::RunTest(const FString& Parameters)
{
	UTemporalRecorderComponent* A = NewObject<UTemporalRecorderComponent>();
	UTemporalRecorderComponent* B = NewObject<UTemporalRecorderComponent>();
	UTemporalRecorderComponent* C = NewObject<UTemporalRecorderComponent>();
	A->LinkWith(B);
	B->LinkWith(C);

	TSet<UTemporalRecorderComponent*> FromA;
	A->GatherGroup(FromA);
	TestEqual(TEXT("El grupo de A tiene 3"), FromA.Num(), 3);
	TestTrue(TEXT("El grupo de A incluye a C (enlace transitivo)"), FromA.Contains(C));

	TSet<UTemporalRecorderComponent*> FromC;
	C->GatherGroup(FromC);
	TestEqual(TEXT("El grupo de C tiene 3 (enlace en ambos sentidos)"), FromC.Num(), 3);
	TestTrue(TEXT("El grupo de C incluye a A"), FromC.Contains(A));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaGroupIgnoresDestroyed, "Entropia.Group.IgnoresDestroyed", EntropiaTestFlags::Flags)
bool FEntropiaGroupIgnoresDestroyed::RunTest(const FString& Parameters)
{
	UTemporalRecorderComponent* A = NewObject<UTemporalRecorderComponent>();
	UTemporalRecorderComponent* B = NewObject<UTemporalRecorderComponent>();
	A->LinkWith(B);
	B->MarkAsGarbage();

	TSet<UTemporalRecorderComponent*> Group;
	A->GatherGroup(Group);
	TestEqual(TEXT("Un enlace destruido no entra al grupo"), Group.Num(), 1);
	TestTrue(TEXT("El grupo conserva a A"), Group.Contains(A));
	return true;
}

// ---------------------------------------------------------------------------
// Revisión final: apuntar y tiempo real

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaAimPrefersCrosshair, "Entropia.Aim.PrefersCrosshairOverBigNearby", EntropiaTestFlags::Flags)
bool FEntropiaAimPrefersCrosshair::RunTest(const FString& Parameters)
{
	// Plataforma de 3 m bajo el personaje: su esfera cubre la mira (borde -9°) pero su centro está a 30°.
	// Bala a 10 m justo en la mira: borde -1°, centro 0.5°. Debe ganar la bala.
	TestTrue(TEXT("La bala en la mira le gana a la plataforma grande cercana"),
		UTemporalAimComponent::IsBetterAim(-1.f, 0.5f, -9.f, 30.f));
	TestFalse(TEXT("Y no al revés"),
		UTemporalAimComponent::IsBetterAim(-9.f, 30.f, -1.f, 0.5f));
	// Si ninguno cubre la mira, gana el borde más cercano.
	TestTrue(TEXT("Fuera de la mira gana el borde más cercano"),
		UTemporalAimComponent::IsBetterAim(1.f, 3.f, 2.5f, 2.6f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaRealDeltaClamped, "Entropia.Time.RealDeltaClamped", EntropiaTestFlags::Flags)
bool FEntropiaRealDeltaClamped::RunTest(const FString& Parameters)
{
	const double Saved = FApp::GetDeltaTime();
	FApp::SetDeltaTime(0.5);
	TestEqual(TEXT("Un tirón de 0.5 s cuenta como 0.1 s"), TemporalFX::RealDelta(), 0.1f);
	FApp::SetDeltaTime(1.0 / 60.0);
	TestEqual(TEXT("Un frame normal no cambia"), TemporalFX::RealDelta(), 1.f / 60.f, 1e-5f);
	FApp::SetDeltaTime(Saved);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 2 · Fractura Voronoi

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFractureBoxVolume, "Entropia.Fracture.BoxVolume", EntropiaTestFlags::Flags)
bool FEntropiaFractureBoxVolume::RunTest(const FString& Parameters)
{
	const FFractureCell Box = TemporalFracture::MakeBox(FVector(50.f));
	TestEqual(TEXT("Una caja de 100 cm tiene 1,000,000 cm³"), TemporalFracture::CellVolume(Box), 1.0e6f, 1.f);
	TestEqual(TEXT("La caja tiene 6 caras"), Box.Faces.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFractureClipHalves, "Entropia.Fracture.ClipHalves", EntropiaTestFlags::Flags)
bool FEntropiaFractureClipHalves::RunTest(const FString& Parameters)
{
	const FFractureCell Box = TemporalFracture::MakeBox(FVector(50.f));
	FFractureCell Half;
	TestTrue(TEXT("Cortar por la mitad deja algo"), TemporalFracture::ClipCell(Box, FPlane(FVector(1.f, 0.f, 0.f), 0.f), Half));
	TestEqual(TEXT("La mitad pesa la mitad"), TemporalFracture::CellVolume(Half), 5.0e5f, 1.f);
	int32 Interior = 0;
	for (const FFracturePolygon& Face : Half.Faces)
	{
		Interior += Face.bInterior ? 1 : 0;
	}
	TestEqual(TEXT("Queda una sola cara interior (la tapa)"), Interior, 1);
	FFractureCell Nothing;
	TestFalse(TEXT("Cortar fuera de la caja deja vacío"), TemporalFracture::ClipCell(Box, FPlane(FVector(1.f, 0.f, 0.f), -60.f), Nothing));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFractureVoronoiVolume, "Entropia.Fracture.VoronoiConservesVolume", EntropiaTestFlags::Flags)
bool FEntropiaFractureVoronoiVolume::RunTest(const FString& Parameters)
{
	// El muro de la sala 4: 40 × 500 × 300 cm.
	const FVector Half(20.f, 250.f, 150.f);
	const TArray<FVector> Seeds = TemporalFracture::MakeSeeds(Half, 10, FVector::ZeroVector, 7);
	const TArray<FFractureCell> Cells = TemporalFracture::VoronoiFracture(Half, Seeds);
	TestEqual(TEXT("Una celda por semilla"), Cells.Num(), 10);
	float Total = 0.f;
	for (const FFractureCell& Cell : Cells)
	{
		const float Volume = TemporalFracture::CellVolume(Cell);
		TestTrue(TEXT("Ninguna celda vacía"), Volume > 1.f);
		Total += Volume;
	}
	const float Expected = 8.f * Half.X * Half.Y * Half.Z;
	TestEqual(TEXT("Los pedazos suman el volumen del muro"), Total, Expected, Expected * 0.005f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFractureDeterministic, "Entropia.Fracture.Deterministic", EntropiaTestFlags::Flags)
bool FEntropiaFractureDeterministic::RunTest(const FString& Parameters)
{
	const FVector Half(50.f);
	const TArray<FVector> A = TemporalFracture::MakeSeeds(Half, 10, FVector(40.f, 0.f, 0.f), 1234);
	const TArray<FVector> B = TemporalFracture::MakeSeeds(Half, 10, FVector(40.f, 0.f, 0.f), 1234);
	TestEqual(TEXT("Mismo número de semillas"), A.Num(), B.Num());
	bool bSame = A.Num() == B.Num();
	for (int32 i = 0; bSame && i < A.Num(); ++i)
	{
		bSame = A[i].Equals(B[i], 0.001f);
	}
	TestTrue(TEXT("Misma semilla = mismos puntos (el rearmado es reproducible)"), bSame);
	for (const FVector& P : A)
	{
		TestTrue(TEXT("Las semillas quedan dentro de la caja"), FMath::Abs(P.X) <= Half.X && FMath::Abs(P.Y) <= Half.Y && FMath::Abs(P.Z) <= Half.Z);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Fase 2 · Escombros reversibles (movimiento analítico)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDebrisStartsAtOrigin, "Entropia.Debris.StartsAtOrigin", EntropiaTestFlags::Flags)
bool FEntropiaDebrisStartsAtOrigin::RunTest(const FString& Parameters)
{
	FDebrisParticle P;
	P.Origin = FVector(10.f, 20.f, 300.f);
	P.Velocity = FVector(200.f, -100.f, 350.f);
	P.Size = 8.f;
	TestTrue(TEXT("En T = 0 la astilla está en el punto del golpe (rebobinar la regresa ahí)"),
		UTemporalDebrisComponent::EvaluateChip(P, 0.f, -1000.f).Equals(P.Origin, 0.01f));
	const FVector A = UTemporalDebrisComponent::EvaluateChip(P, 0.3f, -1000.f);
	const FVector B = UTemporalDebrisComponent::EvaluateChip(P, 0.3f, -1000.f);
	TestTrue(TEXT("Mismo T = misma posición (determinista: sirve para reproducir hacia atrás)"), A.Equals(B, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDebrisStopsAtFloor, "Entropia.Debris.StopsAtFloor", EntropiaTestFlags::Flags)
bool FEntropiaDebrisStopsAtFloor::RunTest(const FString& Parameters)
{
	FDebrisParticle P;
	P.Origin = FVector(0.f, 0.f, 100.f);
	P.Velocity = FVector(300.f, 0.f, 200.f);
	P.Size = 10.f;
	const FVector Late = UTemporalDebrisComponent::EvaluateChip(P, 10.f, 0.f);
	TestEqual(TEXT("Termina apoyada en el piso"), static_cast<float>(Late.Z), 5.f, 0.01f);
	const FVector Later = UTemporalDebrisComponent::EvaluateChip(P, 20.f, 0.f);
	TestTrue(TEXT("Ya en el piso no se sigue deslizando"), Late.Equals(Later, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDebrisDustScaleShape, "Entropia.Debris.DustScaleShape", EntropiaTestFlags::Flags)
bool FEntropiaDebrisDustScaleShape::RunTest(const FString& Parameters)
{
	const float Life = 2.5f;
	TestEqual(TEXT("El polvo nace de nada"), UTemporalDebrisComponent::DustScale(0.f, Life), 0.f, 0.001f);
	TestEqual(TEXT("Y se va a nada"), UTemporalDebrisComponent::DustScale(Life, Life), 0.f, 0.001f);
	float Peak = 0.f;
	for (int32 i = 0; i <= 100; ++i)
	{
		Peak = FMath::Max(Peak, UTemporalDebrisComponent::DustScale(Life * i / 100.f, Life));
	}
	TestTrue(TEXT("Llega a su máximo cerca del primer cuarto de su vida"), UTemporalDebrisComponent::DustScale(Life * 0.25f, Life) > 0.9f * Peak);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 2 · Ondas de distorsión

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaPulseRadiusGrows, "Entropia.Pulse.RadiusGrows", EntropiaTestFlags::Flags)
bool FEntropiaPulseRadiusGrows::RunTest(const FString& Parameters)
{
	FTemporalPulse P;
	P.Duration = 0.6f;
	P.MaxRadius = 0.35f;
	P.Age = 0.f;
	TestEqual(TEXT("La onda nace en el centro"), FTemporalPulseSet::RadiusAt(P), 0.f, 0.0001f);
	P.Age = P.Duration;
	TestEqual(TEXT("Termina en su radio máximo"), FTemporalPulseSet::RadiusAt(P), 0.35f, 0.0001f);
	P.Age = P.Duration * 0.5f;
	const float Mid = FTemporalPulseSet::RadiusAt(P);
	TestTrue(TEXT("Se abre rápido y se frena al final (ease-out)"), Mid > 0.8f * 0.35f && Mid < 0.95f * 0.35f);
	TestTrue(TEXT("La fuerza se apaga al final"), FTemporalPulseSet::StrengthAt(P) < P.Strength);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaPulseReplacesOldest, "Entropia.Pulse.ReplacesOldest", EntropiaTestFlags::Flags)
bool FEntropiaPulseReplacesOldest::RunTest(const FString& Parameters)
{
	FTemporalPulseSet Set;
	for (int32 i = 0; i < 5; ++i)
	{
		FTemporalPulse P;
		P.World = FVector(i, 0.f, 0.f);
		P.Duration = 10.f;
		Set.Add(P);
		Set.Advance(0.1f);
	}
	TestEqual(TEXT("Nunca hay más de 4 ondas"), Set.Pulses.Num(), FTemporalPulseSet::MaxPulses);
	bool bFirstStill = false;
	for (const FTemporalPulse& P : Set.Pulses)
	{
		bFirstStill |= FMath::IsNearlyZero(P.World.X);
	}
	TestFalse(TEXT("La onda más vieja fue la reemplazada"), bFirstStill);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaPulseExpires, "Entropia.Pulse.Expires", EntropiaTestFlags::Flags)
bool FEntropiaPulseExpires::RunTest(const FString& Parameters)
{
	FTemporalPulseSet Set;
	FTemporalPulse P;
	P.Duration = 0.5f;
	Set.Add(P);
	Set.Advance(0.51f);
	TestEqual(TEXT("Una onda vencida desaparece"), Set.Pulses.Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------
// Tour (Fase 2): la estela no apila copias si estás quieto

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaTrailSkipsWhenStill, "Entropia.Trail.SkipsWhenStill", EntropiaTestFlags::Flags)
bool FEntropiaTrailSkipsWhenStill::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("La primera copia siempre sale"), UTemporalTrailComponent::ShouldEmit(false, FVector::ZeroVector, FVector::ZeroVector, 25.f));
	TestFalse(TEXT("Quieto (10 cm) no deja otra copia encima de ti"), UTemporalTrailComponent::ShouldEmit(true, FVector::ZeroVector, FVector(10.f, 0.f, 0.f), 25.f));
	TestTrue(TEXT("Moviéndote (40 cm) sí deja copia"), UTemporalTrailComponent::ShouldEmit(true, FVector::ZeroVector, FVector(40.f, 0.f, 0.f), 25.f));
	return true;
}

// ---------------------------------------------------------------------------
// Revisión final Fase 2: los pedazos se dibujan con la cara hacia afuera

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFractureTrianglesFaceOut, "Entropia.Fracture.TrianglesFaceOutward", EntropiaTestFlags::Flags)
bool FEntropiaFractureTrianglesFaceOut::RunTest(const FString& Parameters)
{
	const FVector Half(20.f, 250.f, 150.f);
	const TArray<FFractureCell> Cells = TemporalFracture::VoronoiFracture(Half, TemporalFracture::MakeSeeds(Half, 8, FVector(0.f, 100.f, 50.f), 3));
	int32 Inward = 0;
	int32 Checked = 0;
	for (const FFractureCell& Cell : Cells)
	{
		const FVector Centroid = TemporalFracture::CellCentroid(Cell);
		for (const FFracturePolygon& Face : Cell.Faces)
		{
			FVector FaceCenter = FVector::ZeroVector;
			for (const FVector& P : Face.Points)
			{
				FaceCenter += P;
			}
			FaceCenter /= Face.Points.Num();
			const FVector Out = FaceCenter - Centroid;

			if (FVector::DotProduct(TemporalFracture::FaceNormal(Face), Out) <= 0.f)
			{
				++Inward;
			}
			TArray<int32> Triangles;
			TemporalFracture::AppendFanTriangles(0, Face.Points.Num(), Triangles);
			for (int32 t = 0; t + 2 < Triangles.Num(); t += 3)
			{
				const FVector& A = Face.Points[Triangles[t]];
				const FVector& B = Face.Points[Triangles[t + 1]];
				const FVector& C = Face.Points[Triangles[t + 2]];
				// Convención de Unreal (mano izquierda): la cara visible de (A, B, C) apunta a (C − A) × (B − A).
				if (FVector::DotProduct(FVector::CrossProduct(C - A, B - A), Out) <= 0.f)
				{
					++Inward;
				}
				++Checked;
			}
		}
	}
	TestTrue(TEXT("Hay triángulos que revisar"), Checked > 0);
	TestEqual(TEXT("Ningún triángulo ni normal mira hacia adentro"), Inward, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDebrisFollowsShardClock, "Entropia.Debris.FollowsShardClock", EntropiaTestFlags::Flags)
bool FEntropiaDebrisFollowsShardClock::RunTest(const FString& Parameters)
{
	const float Life = 2.5f;
	// Se rompió hace 5 s: el polvo ya se fue.
	TestEqual(TEXT("Con 5 s de historia el polvo ya terminó"), UTemporalDebrisComponent::TimeFromShardHistory(5.f, Life), Life, 0.0001f);
	// Al rebobinar, el polvo NO vuelve hasta que los pedazos regresan a los últimos 2.5 s (van juntos).
	TestEqual(TEXT("Rebobinando, a los 4 s de historia el polvo sigue sin volver"), UTemporalDebrisComponent::TimeFromShardHistory(4.f, Life), Life, 0.0001f);
	TestEqual(TEXT("A 1 s del golpe, el polvo está a 1 s del golpe"), UTemporalDebrisComponent::TimeFromShardHistory(1.f, Life), 1.f, 0.0001f);
	TestEqual(TEXT("Sin historia, el polvo está en el golpe"), UTemporalDebrisComponent::TimeFromShardHistory(-0.1f, Life), 0.f, 0.0001f);
	return true;
}

// ---------------------------------------------------------------------------
// Pulido: campos de tiempo detenido (burbuja)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFieldSetUpdatesSameOwner, "Entropia.Field.UpdatesSameOwner", EntropiaTestFlags::Flags)
bool FEntropiaFieldSetUpdatesSameOwner::RunTest(const FString& Parameters)
{
	FTemporalFieldSet Set;
	Set.Set(7, FVector(0.f), 100.f, 1.f);
	Set.Set(7, FVector(50.f, 0.f, 0.f), 300.f, 0.5f);
	TestEqual(TEXT("La misma burbuja ocupa un solo lugar"), Set.Fields.Num(), 1);
	TestEqual(TEXT("Y se actualiza su radio"), Set.Fields[0].Radius, 300.f, 0.001f);
	Set.Remove(7);
	TestEqual(TEXT("Al quitarla no queda nada"), Set.Fields.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaFieldSetCapsAndReplacesOldest, "Entropia.Field.CapsAndReplacesOldest", EntropiaTestFlags::Flags)
bool FEntropiaFieldSetCapsAndReplacesOldest::RunTest(const FString& Parameters)
{
	FTemporalFieldSet Set;
	Set.Set(1, FVector(0.f), 100.f, 1.f);
	Set.Set(2, FVector(0.f), 100.f, 1.f);
	Set.Set(3, FVector(0.f), 100.f, 1.f);
	TestEqual(TEXT("Nunca hay más campos que lugares en el material"), Set.Fields.Num(), FTemporalFieldSet::MaxFields);
	bool bHasFirst = false;
	for (const FTemporalField& Field : Set.Fields)
	{
		bHasFirst |= Field.Id == 1;
	}
	TestFalse(TEXT("Si hay demasiadas burbujas, la más vieja deja de pintarse"), bHasFirst);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 3 · Árbitro del tiempo: cada poder pide una velocidad y gana la más lenta

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDilationSlowestWins, "Entropia.Dilation.SlowestWins", EntropiaTestFlags::Flags)
bool FEntropiaDilationSlowestWins::RunTest(const FString& Parameters)
{
	FTemporalDilationLedger Ledger;
	int32 Key = 0, OjoLento = 0, Burbuja = 0;
	TestTrue(TEXT("Primer pedido cambia la velocidad"), Ledger.Request(&Key, &OjoLento, 0.25f));
	TestEqual(TEXT("Ojo lento: 0.25"), Ledger.Get(&Key), 0.25f, 0.0001f);
	Ledger.Request(&Key, &Burbuja, 0.f);
	TestEqual(TEXT("La burbuja congela: gana la más lenta"), Ledger.Get(&Key), 0.f, 0.0001f);
	TestTrue(TEXT("Soltar la burbuja cambia la velocidad"), Ledger.Release(&Key, &Burbuja));
	TestEqual(TEXT("Vuelve la del Ojo lento, no 1"), Ledger.Get(&Key), 0.25f, 0.0001f);
	Ledger.Release(&Key, &OjoLento);
	TestEqual(TEXT("Sin pedidos: tiempo normal"), Ledger.Get(&Key), 1.f, 0.0001f);
	TestFalse(TEXT("Y la clave ya no se guarda"), Ledger.Has(&Key));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDilationReleaseSource, "Entropia.Dilation.ReleaseSource", EntropiaTestFlags::Flags)
bool FEntropiaDilationReleaseSource::RunTest(const FString& Parameters)
{
	FTemporalDilationLedger Ledger;
	int32 K1 = 0, K2 = 0, A = 0, B = 0;
	Ledger.Request(&K1, &A, 0.5f);
	Ledger.Request(&K2, &A, 0.f);
	Ledger.Request(&K2, &B, 0.3f);
	const TArray<const void*> Changed = Ledger.ReleaseSource(&A);
	TestEqual(TEXT("Cambiaron las dos claves"), Changed.Num(), 2);
	TestEqual(TEXT("K1 vuelve a 1"), Ledger.Get(&K1), 1.f, 0.0001f);
	TestEqual(TEXT("K2 queda con el pedido de B"), Ledger.Get(&K2), 0.3f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDilationActorRate, "Entropia.Dilation.ActorRateIsSlowestOfOwnAndWorld", EntropiaTestFlags::Flags)
bool FEntropiaDilationActorRate::RunTest(const FString& Parameters)
{
	// Lo que se escribe en CustomTimeDilation para que el ritmo real (propio × mundo) sea el MÁS LENTO de los dos.
	TestEqual(TEXT("Ojo lento (0.25) en Sobremarcha (0.25): la bala va a 0.25, no a 0.0625"), UTemporalDilationSubsystem::ActorTimeFor(0.25f, 0.25f), 1.f, 0.0001f);
	TestEqual(TEXT("Sin cámara lenta global: su propia velocidad"), UTemporalDilationSubsystem::ActorTimeFor(0.25f, 1.f), 0.25f, 0.0001f);
	TestEqual(TEXT("Congelada sigue congelada"), UTemporalDilationSubsystem::ActorTimeFor(0.f, 0.3f), 0.f, 0.0001f);
	TestEqual(TEXT("Más lenta que el mundo: 0.1 real con el mundo a 0.25"), UTemporalDilationSubsystem::ActorTimeFor(0.1f, 0.25f), 0.4f, 0.0001f);
	TestEqual(TEXT("Sin pedidos: 1 (el mundo ya la frena)"), UTemporalDilationSubsystem::ActorTimeFor(1.f, 0.25f), 1.f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDilationPlayerCompensation, "Entropia.Dilation.PlayerCompensation", EntropiaTestFlags::Flags)
bool FEntropiaDilationPlayerCompensation::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Mundo a 0.25: el jugador va x4 para verse normal"), UTemporalDilationSubsystem::PlayerTimeFor(1.f, 0.25f), 4.f, 0.0001f);
	TestEqual(TEXT("Jugador congelado sigue congelado"), UTemporalDilationSubsystem::PlayerTimeFor(0.f, 0.25f), 0.f, 0.0001f);
	TestEqual(TEXT("Sin cámara lenta: 1"), UTemporalDilationSubsystem::PlayerTimeFor(1.f, 1.f), 1.f, 0.0001f);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 3 · Dash

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDashDirection, "Entropia.Dash.DirectionFallsBackToFacing", EntropiaTestFlags::Flags)
bool FEntropiaDashDirection::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Quieto: dash hacia donde miras"),
		UTemporalDashComponent::ComputeDashDirection(FVector::ZeroVector, FVector(0.f, 1.f, 0.f)).Equals(FVector(0.f, 1.f, 0.f), 0.001f));
	TestTrue(TEXT("Moviéndote: hacia donde te mueves, plano y normalizado"),
		UTemporalDashComponent::ComputeDashDirection(FVector(3.f, 0.f, 1.f), FVector(0.f, 1.f, 0.f)).Equals(FVector(1.f, 0.f, 0.f), 0.001f));
	TestTrue(TEXT("Mirando hacia arriba: el dash sigue siendo horizontal"),
		UTemporalDashComponent::ComputeDashDirection(FVector::ZeroVector, FVector(0.6f, 0.f, 0.8f)).Equals(FVector(1.f, 0.f, 0.f), 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaDashCooldown, "Entropia.Dash.CooldownAndAirDash", EntropiaTestFlags::Flags)
bool FEntropiaDashCooldown::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("En recarga no hay dash"), UTemporalDashComponent::CanDash(0.3f, 0.6f, false, false));
	TestTrue(TEXT("Pasada la recarga, sí"), UTemporalDashComponent::CanDash(0.7f, 0.6f, false, false));
	TestFalse(TEXT("En el aire, solo uno por salto"), UTemporalDashComponent::CanDash(0.7f, 0.6f, true, true));
	TestTrue(TEXT("En el aire, el primero sí"), UTemporalDashComponent::CanDash(0.7f, 0.6f, true, false));
	return true;
}

// ---------------------------------------------------------------------------
// Fase 3 · Vida

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaHealthDeathOnce, "Entropia.Health.DamageAndDeathOnce", EntropiaTestFlags::Flags)
bool FEntropiaHealthDeathOnce::RunTest(const FString& Parameters)
{
	UTemporalHealthComponent* Health = NewObject<UTemporalHealthComponent>();
	int32 Deaths = 0;
	Health->OnDeath.AddLambda([&Deaths]() { ++Deaths; });
	for (int32 i = 0; i < 5; ++i)
	{
		TestTrue(TEXT("Cada bala de 20 hace daño"), Health->ApplyDamage(20.f));
	}
	TestTrue(TEXT("Con 5 balas mueres"), Health->IsDead());
	TestEqual(TEXT("La vida no baja de 0"), Health->Health, 0.f);
	TestFalse(TEXT("Muerto ya no recibe daño"), Health->ApplyDamage(20.f));
	TestEqual(TEXT("Mueres una sola vez"), Deaths, 1);
	Health->ResetHealth();
	TestFalse(TEXT("Al reaparecer estás vivo"), Health->IsDead());
	TestEqual(TEXT("Y con la vida llena"), Health->Health, 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaHealthRegen, "Entropia.Health.RegenAfterDelay", EntropiaTestFlags::Flags)
bool FEntropiaHealthRegen::RunTest(const FString& Parameters)
{
	UTemporalHealthComponent* Health = NewObject<UTemporalHealthComponent>();
	Health->ApplyDamage(40.f);
	Health->Advance(3.9f);
	TestEqual(TEXT("Antes de 4 s sin daño no se recupera"), Health->Health, 60.f, 0.01f);
	Health->Advance(1.f);
	TestEqual(TEXT("Después se recupera a 25/s (0.9 s → +22.5)"), Health->Health, 82.5f, 0.01f);
	Health->Advance(10.f);
	TestEqual(TEXT("Nunca pasa de 100"), Health->Health, 100.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaHealthInvulnerable, "Entropia.Health.InvulnerableBlocksDamage", EntropiaTestFlags::Flags)
bool FEntropiaHealthInvulnerable::RunTest(const FString& Parameters)
{
	UTemporalHealthComponent* Health = NewObject<UTemporalHealthComponent>();
	Health->SetInvulnerable(0.25f);
	TestFalse(TEXT("Durante el dash la bala no hace daño"), Health->ApplyDamage(20.f));
	TestEqual(TEXT("La vida sigue llena"), Health->Health, 100.f);
	Health->Advance(0.3f);
	TestTrue(TEXT("Pasado el instante, sí"), Health->ApplyDamage(20.f));
	TestEqual(TEXT("Queda en 80"), Health->Health, 80.f);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 3 · Bala contra cápsula (pegar y esquiva perfecta)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaCombatSegmentCapsule, "Entropia.Combat.SegmentHitsCapsule", EntropiaTestFlags::Flags)
bool FEntropiaCombatSegmentCapsule::RunTest(const FString& Parameters)
{
	// Cápsula del jugador: 96 de media altura, 42 de radio, en el origen.
	float Fraction = -1.f;
	TestTrue(TEXT("De frente pega"), TemporalCombat::SegmentHitsCapsule(FVector(-500.f, 0.f, 0.f), FVector(500.f, 0.f, 0.f), FVector::ZeroVector, 96.f, 42.f, Fraction));
	TestEqual(TEXT("Pega al llegar al borde (458 de 1000)"), Fraction, 0.458f, 0.002f);
	TestFalse(TEXT("A 1 m de lado no pega"), TemporalCombat::SegmentHitsCapsule(FVector(-500.f, 100.f, 0.f), FVector(500.f, 100.f, 0.f), FVector::ZeroVector, 96.f, 42.f, Fraction));
	TestFalse(TEXT("Por encima de la cabeza no pega"), TemporalCombat::SegmentHitsCapsule(FVector(-500.f, 0.f, 200.f), FVector(500.f, 0.f, 200.f), FVector::ZeroVector, 96.f, 42.f, Fraction));
	TestFalse(TEXT("Si se queda corta no pega"), TemporalCombat::SegmentHitsCapsule(FVector(-500.f, 0.f, 0.f), FVector(-100.f, 0.f, 0.f), FVector::ZeroVector, 96.f, 42.f, Fraction));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaCombatTimeToHit, "Entropia.Combat.TimeToHitCapsule", EntropiaTestFlags::Flags)
bool FEntropiaCombatTimeToHit::RunTest(const FString& Parameters)
{
	const float T = TemporalCombat::TimeToHitCapsule(FVector(-500.f, 0.f, 0.f), FVector(2500.f, 0.f, 0.f), FVector::ZeroVector, 96.f, 42.f, 0.35f);
	TestEqual(TEXT("A 5 m y 25 m/s pega en ~0.183 s"), T, 0.1832f, 0.002f);
	TestEqual(TEXT("Alejándose no pega"), TemporalCombat::TimeToHitCapsule(FVector(-500.f, 0.f, 0.f), FVector(-2500.f, 0.f, 0.f), FVector::ZeroVector, 96.f, 42.f, 0.35f), -1.f);
	TestEqual(TEXT("Si tarda más que la ventana, no cuenta"), TemporalCombat::TimeToHitCapsule(FVector(-2000.f, 0.f, 0.f), FVector(2500.f, 0.f, 0.f), FVector::ZeroVector, 96.f, 42.f, 0.35f), -1.f);
	return true;
}

// ---------------------------------------------------------------------------
// Fase 3 · Remates

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaTakedownCondition, "Entropia.Takedown.BehindOrFrozen", EntropiaTestFlags::Flags)
bool FEntropiaTakedownCondition::RunTest(const FString& Parameters)
{
	// Maniquí en el origen mirando hacia +X. Remate: a menos de 180 cm y (a su espalda > 120° o congelado).
	const FVector Target = FVector::ZeroVector;
	const FVector Forward(1.f, 0.f, 0.f);
	TestTrue(TEXT("A 1.5 m por la espalda: sí"), UTakedownComponent::CanTakedown(FVector(-150.f, 0.f, 0.f), Target, Forward, false, 180.f, 120.f));
	TestFalse(TEXT("A 1.5 m de frente: no"), UTakedownComponent::CanTakedown(FVector(150.f, 0.f, 0.f), Target, Forward, false, 180.f, 120.f));
	TestTrue(TEXT("De frente pero congelado: sí"), UTakedownComponent::CanTakedown(FVector(150.f, 0.f, 0.f), Target, Forward, true, 180.f, 120.f));
	TestFalse(TEXT("Por la espalda pero a 2.5 m: no"), UTakedownComponent::CanTakedown(FVector(-250.f, 0.f, 0.f), Target, Forward, false, 180.f, 120.f));
	TestFalse(TEXT("De lado (90°): no"), UTakedownComponent::CanTakedown(FVector(0.f, 150.f, 0.f), Target, Forward, false, 180.f, 120.f));
	TestFalse(TEXT("Congelado pero lejos: no"), UTakedownComponent::CanTakedown(FVector(400.f, 0.f, 0.f), Target, Forward, true, 180.f, 120.f));
	return true;
}

#endif
