// ENTROPÍA · Geometría de combate

#include "TemporalCombat.h"

namespace TemporalCombat
{
	bool SegmentHitsCapsule(const FVector& A, const FVector& B, const FVector& Center, float HalfHeight, float Radius, float& OutFraction)
	{
		// La cápsula es el conjunto de puntos a menos de Radius de su eje (un segmento vertical).
		const float AxisHalf = FMath::Max(HalfHeight - Radius, 0.f);
		const FVector Bottom = Center - FVector(0.f, 0.f, AxisHalf);
		const FVector Top = Center + FVector(0.f, 0.f, AxisHalf);
		auto DistanceAt = [&](float T)
		{
			return FMath::PointDistToSegment(FMath::Lerp(A, B, T), Bottom, Top);
		};

		if (DistanceAt(0.f) <= Radius)
		{
			OutFraction = 0.f;
			return true;
		}

		// El punto más cercano entre el tramo y el eje: si ni ahí toca, no toca nunca.
		FVector OnPath, OnAxis;
		FMath::SegmentDistToSegmentSafe(A, B, Bottom, Top, OnPath, OnAxis);
		if (FVector::Dist(OnPath, OnAxis) > Radius)
		{
			return false;
		}
		const float PathLength = FVector::Dist(A, B);
		float Hi = PathLength > KINDA_SMALL_NUMBER ? FVector::Dist(A, OnPath) / PathLength : 0.f;

		// La distancia al eje a lo largo del tramo es convexa: la entrada está entre 0 y el punto más cercano.
		float Lo = 0.f;
		for (int32 Step = 0; Step < 24; ++Step)
		{
			const float Mid = 0.5f * (Lo + Hi);
			if (DistanceAt(Mid) <= Radius)
			{
				Hi = Mid;
			}
			else
			{
				Lo = Mid;
			}
		}
		OutFraction = Hi;
		return true;
	}

	float TimeToHitCapsule(const FVector& Pos, const FVector& Vel, const FVector& Center, float HalfHeight, float Radius, float MaxSeconds)
	{
		float Fraction = 0.f;
		if (MaxSeconds <= 0.f || !SegmentHitsCapsule(Pos, Pos + Vel * MaxSeconds, Center, HalfHeight, Radius, Fraction))
		{
			return -1.f;
		}
		return Fraction * MaxSeconds;
	}
}
