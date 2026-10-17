// ENTROPÍA · Fractura Voronoi

#include "TemporalFracture.h"

namespace EntropiaFracture
{
	constexpr float PlaneEpsilon = 1.0e-3f;
	constexpr float WeldDistance = 0.01f;

	FVector PolygonNormal(const TArray<FVector>& Points)
	{
		// Newell: robusto aunque el polígono tenga puntos casi alineados.
		FVector Normal = FVector::ZeroVector;
		for (int32 i = 0; i < Points.Num(); ++i)
		{
			const FVector& A = Points[i];
			const FVector& B = Points[(i + 1) % Points.Num()];
			Normal.X += (A.Y - B.Y) * (A.Z + B.Z);
			Normal.Y += (A.Z - B.Z) * (A.X + B.X);
			Normal.Z += (A.X - B.X) * (A.Y + B.Y);
		}
		return Normal.GetSafeNormal();
	}

	FVector PolygonCenter(const TArray<FVector>& Points)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FVector& P : Points)
		{
			Sum += P;
		}
		return Points.Num() > 0 ? Sum / Points.Num() : Sum;
	}

	void AddUnique(TArray<FVector>& Points, const FVector& P)
	{
		for (const FVector& Existing : Points)
		{
			if (Existing.Equals(P, WeldDistance))
			{
				return;
			}
		}
		Points.Add(P);
	}
}

FFractureCell TemporalFracture::MakeBox(const FVector& H)
{
	using namespace EntropiaFracture;
	const FVector C[8] = {
		FVector(-H.X, -H.Y, -H.Z), FVector(H.X, -H.Y, -H.Z), FVector(H.X, H.Y, -H.Z), FVector(-H.X, H.Y, -H.Z),
		FVector(-H.X, -H.Y, H.Z), FVector(H.X, -H.Y, H.Z), FVector(H.X, H.Y, H.Z), FVector(-H.X, H.Y, H.Z),
	};
	const int32 Quads[6][4] = { { 1, 2, 6, 5 }, { 0, 4, 7, 3 }, { 3, 7, 6, 2 }, { 0, 1, 5, 4 }, { 4, 5, 6, 7 }, { 0, 3, 2, 1 } };

	FFractureCell Box;
	for (const auto& Quad : Quads)
	{
		FFracturePolygon Face;
		for (const int32 Index : Quad)
		{
			Face.Points.Add(C[Index]);
		}
		// Asegurar normal hacia afuera (la caja está centrada en el origen).
		if (FVector::DotProduct(PolygonNormal(Face.Points), PolygonCenter(Face.Points)) < 0.f)
		{
			Algo::Reverse(Face.Points);
		}
		Box.Faces.Add(MoveTemp(Face));
	}
	return Box;
}

bool TemporalFracture::ClipCell(const FFractureCell& In, const FPlane& Plane, FFractureCell& Out)
{
	using namespace EntropiaFracture;
	Out.Faces.Reset();
	TArray<FVector> CapPoints;
	bool bCut = false;

	for (const FFracturePolygon& Face : In.Faces)
	{
		FFracturePolygon Clipped;
		Clipped.bInterior = Face.bInterior;
		const int32 N = Face.Points.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const FVector& A = Face.Points[i];
			const FVector& B = Face.Points[(i + 1) % N];
			const float DA = Plane.PlaneDot(A);
			const float DB = Plane.PlaneDot(B);
			const bool bAIn = DA <= PlaneEpsilon;
			const bool bBIn = DB <= PlaneEpsilon;

			if (bAIn)
			{
				Clipped.Points.Add(A);
				if (FMath::Abs(DA) <= PlaneEpsilon)
				{
					AddUnique(CapPoints, A);
				}
			}
			// La arista cruza el plano: agregamos el punto de cruce (es parte de la tapa).
			if (bAIn != bBIn)
			{
				const float T = DA / (DA - DB);
				const FVector Cross = A + (B - A) * T;
				Clipped.Points.Add(Cross);
				AddUnique(CapPoints, Cross);
				bCut = true;
			}
		}

		// Quitar puntos repetidos seguidos que deja el corte.
		TArray<FVector> Clean;
		for (const FVector& P : Clipped.Points)
		{
			if (Clean.Num() == 0 || !Clean.Last().Equals(P, WeldDistance))
			{
				Clean.Add(P);
			}
		}
		if (Clean.Num() > 1 && Clean.Last().Equals(Clean[0], WeldDistance))
		{
			Clean.Pop();
		}
		if (Clean.Num() >= 3)
		{
			Clipped.Points = MoveTemp(Clean);
			Out.Faces.Add(MoveTemp(Clipped));
		}
	}

	if (Out.Faces.Num() == 0)
	{
		return false;
	}

	// Tapa interior: los puntos del corte ordenados alrededor de su centro, mirando hacia +normal del plano (afuera).
	if (bCut && CapPoints.Num() >= 3)
	{
		const FVector Normal = FVector(Plane.X, Plane.Y, Plane.Z).GetSafeNormal();
		const FVector Center = PolygonCenter(CapPoints);
		FVector U, V;
		Normal.FindBestAxisVectors(U, V);
		if (FVector::DotProduct(FVector::CrossProduct(U, V), Normal) < 0.f)
		{
			V = -V;
		}
		CapPoints.Sort([&](const FVector& A, const FVector& B)
		{
			const FVector DA = A - Center;
			const FVector DB = B - Center;
			return FMath::Atan2(FVector::DotProduct(DA, V), FVector::DotProduct(DA, U)) < FMath::Atan2(FVector::DotProduct(DB, V), FVector::DotProduct(DB, U));
		});
		FFracturePolygon Cap;
		Cap.Points = MoveTemp(CapPoints);
		Cap.bInterior = true;
		Out.Faces.Add(MoveTemp(Cap));
	}
	return true;
}

TArray<FFractureCell> TemporalFracture::VoronoiFracture(const FVector& HalfSize, const TArray<FVector>& Seeds)
{
	TArray<FFractureCell> Cells;
	const FFractureCell Box = MakeBox(HalfSize);
	for (int32 i = 0; i < Seeds.Num(); ++i)
	{
		FFractureCell Cell = Box;
		bool bAlive = true;
		for (int32 j = 0; j < Seeds.Num() && bAlive; ++j)
		{
			if (i == j)
			{
				continue;
			}
			// Bisector entre las dos semillas: nos quedamos con el lado más cercano a la nuestra.
			const FVector Normal = (Seeds[j] - Seeds[i]).GetSafeNormal();
			if (Normal.IsNearlyZero())
			{
				continue;
			}
			const FVector Mid = (Seeds[i] + Seeds[j]) * 0.5f;
			FFractureCell Next;
			bAlive = ClipCell(Cell, FPlane(Normal, FVector::DotProduct(Normal, Mid)), Next);
			Cell = MoveTemp(Next);
		}
		if (bAlive && CellVolume(Cell) > 1.f)
		{
			Cells.Add(MoveTemp(Cell));
		}
	}
	return Cells;
}

TArray<FVector> TemporalFracture::MakeSeeds(const FVector& HalfSize, int32 Count, const FVector& LocalImpact, int32 RandomSeed)
{
	FRandomStream Random(RandomSeed);
	const FVector Inner = HalfSize * 0.95f;
	const FVector Impact(
		FMath::Clamp(LocalImpact.X, -Inner.X, Inner.X),
		FMath::Clamp(LocalImpact.Y, -Inner.Y, Inner.Y),
		FMath::Clamp(LocalImpact.Z, -Inner.Z, Inner.Z));
	const float MinGap = HalfSize.GetMin() * 0.15f;

	TArray<FVector> Seeds;
	for (int32 i = 0; i < Count; ++i)
	{
		const bool bNearImpact = i < Count / 2;
		FVector Candidate = FVector::ZeroVector;
		for (int32 Attempt = 0; Attempt < 12; ++Attempt)
		{
			if (bNearImpact)
			{
				const FVector Spread = HalfSize * 0.4f;
				Candidate = Impact + FVector(Random.FRandRange(-Spread.X, Spread.X), Random.FRandRange(-Spread.Y, Spread.Y), Random.FRandRange(-Spread.Z, Spread.Z));
			}
			else
			{
				Candidate = FVector(Random.FRandRange(-Inner.X, Inner.X), Random.FRandRange(-Inner.Y, Inner.Y), Random.FRandRange(-Inner.Z, Inner.Z));
			}
			Candidate = FVector(FMath::Clamp(Candidate.X, -Inner.X, Inner.X), FMath::Clamp(Candidate.Y, -Inner.Y, Inner.Y), FMath::Clamp(Candidate.Z, -Inner.Z, Inner.Z));

			bool bFarEnough = true;
			for (const FVector& Existing : Seeds)
			{
				bFarEnough &= FVector::Dist(Existing, Candidate) >= MinGap;
			}
			if (bFarEnough)
			{
				break;
			}
		}
		Seeds.Add(Candidate);
	}
	return Seeds;
}

float TemporalFracture::CellVolume(const FFractureCell& Cell)
{
	// Teorema de la divergencia: suma de tetraedros (origen, triángulos en abanico de cada cara).
	double Volume = 0.0;
	for (const FFracturePolygon& Face : Cell.Faces)
	{
		for (int32 i = 1; i + 1 < Face.Points.Num(); ++i)
		{
			Volume += FVector::DotProduct(Face.Points[0], FVector::CrossProduct(Face.Points[i], Face.Points[i + 1])) / 6.0;
		}
	}
	return static_cast<float>(FMath::Abs(Volume));
}

FVector TemporalFracture::CellCentroid(const FFractureCell& Cell)
{
	const TArray<FVector> Vertices = CellVertices(Cell);
	const FVector Pivot = EntropiaFracture::PolygonCenter(Vertices);
	double TotalVolume = 0.0;
	FVector Sum = FVector::ZeroVector;
	for (const FFracturePolygon& Face : Cell.Faces)
	{
		for (int32 i = 1; i + 1 < Face.Points.Num(); ++i)
		{
			const FVector& A = Face.Points[0];
			const FVector& B = Face.Points[i];
			const FVector& C = Face.Points[i + 1];
			const double Tet = FMath::Abs(FVector::DotProduct(A - Pivot, FVector::CrossProduct(B - Pivot, C - Pivot))) / 6.0;
			TotalVolume += Tet;
			Sum += (A + B + C + Pivot) * 0.25f * static_cast<float>(Tet);
		}
	}
	return TotalVolume > 0.0 ? Sum / static_cast<float>(TotalVolume) : Pivot;
}

TArray<FVector> TemporalFracture::CellVertices(const FFractureCell& Cell)
{
	TArray<FVector> Vertices;
	for (const FFracturePolygon& Face : Cell.Faces)
	{
		for (const FVector& P : Face.Points)
		{
			EntropiaFracture::AddUnique(Vertices, P);
		}
	}
	return Vertices;
}
