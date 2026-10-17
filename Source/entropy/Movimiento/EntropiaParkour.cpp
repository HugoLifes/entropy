// ENTROPÍA · Parkour: la parte que decide

#include "EntropiaParkour.h"

namespace EntropiaParkour
{
	EEntropiaTraversal Classify(const FEntropiaObstacle& Obstacle, bool bSprinting)
	{
		if (!Obstacle.bHit || Obstacle.Distance > MaxDistance || Obstacle.Height < MinHeight)
		{
			return EEntropiaTraversal::None;
		}
		if (Obstacle.Height <= VaultMaxHeight)
		{
			if (Obstacle.bLandingClear && Obstacle.Depth <= VaultMaxDepth)
			{
				return EEntropiaTraversal::DashVault;
			}
			if (Obstacle.bLandingClear && bSprinting && Obstacle.Depth <= KongMaxDepth)
			{
				return EEntropiaTraversal::Kong;
			}
			return Obstacle.bTopClear ? EEntropiaTraversal::StepUp : EEntropiaTraversal::None;
		}
		if (Obstacle.Height <= ClimbMaxHeight)
		{
			return Obstacle.bTopClear ? EEntropiaTraversal::Climb : EEntropiaTraversal::None;
		}
		if (Obstacle.Height <= WallRunMaxHeight && bSprinting && Obstacle.bTopClear)
		{
			return EEntropiaTraversal::WallRun;
		}
		return EEntropiaTraversal::None;
	}

	float SmoothAlpha(float A)
	{
		const float T = FMath::Clamp(A, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	FVector PathPoint(EEntropiaTraversal Kind, const FVector& Start, const FVector& Edge, const FVector& End, float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		switch (Kind)
		{
		case EEntropiaTraversal::DashVault:
		case EEntropiaTraversal::Kong:
		{
			// Curva (Bézier cuadrática) que a la mitad pasa por encima del borde.
			const FVector Apex((Start.X + End.X) * 0.5f, (Start.Y + End.Y) * 0.5f, FMath::Max(Edge.Z + VaultClearance, FMath::Max(Start.Z, End.Z)));
			const FVector Control = 2.f * Apex - 0.5f * (Start + End);
			const float U = 1.f - T;
			return U * U * Start + 2.f * U * T * Control + T * T * End;
		}
		case EEntropiaTraversal::StepUp:
		case EEntropiaTraversal::Climb:
		case EEntropiaTraversal::WallRun:
		default:
		{
			// Primero sube pegado a la pared (sin atravesarla) y luego avanza arriba.
			FVector Forward = End - Start;
			Forward.Z = 0.f;
			Forward = Forward.GetSafeNormal();
			FVector Up = Edge - Forward * WallGap;
			Up.Z = End.Z;
			constexpr float Split = 0.65f;
			if (T < Split)
			{
				return FMath::Lerp(Start, Up, SmoothAlpha(T / Split));
			}
			return FMath::Lerp(Up, End, SmoothAlpha((T - Split) / (1.f - Split)));
		}
		}
	}

	float Duration(EEntropiaTraversal Kind)
	{
		switch (Kind)
		{
		case EEntropiaTraversal::DashVault: return 0.55f;
		case EEntropiaTraversal::Kong: return 0.6f;
		case EEntropiaTraversal::StepUp: return 0.45f;
		case EEntropiaTraversal::Climb: return 0.9f;
		case EEntropiaTraversal::WallRun: return 1.1f;
		default: return 0.f;
		}
	}

	const TCHAR* Name(EEntropiaTraversal Kind)
	{
		switch (Kind)
		{
		case EEntropiaTraversal::DashVault: return TEXT("dash_vault");
		case EEntropiaTraversal::Kong: return TEXT("gato");
		case EEntropiaTraversal::StepUp: return TEXT("subir");
		case EEntropiaTraversal::Climb: return TEXT("trepar");
		case EEntropiaTraversal::WallRun: return TEXT("pared");
		default: return TEXT("nada");
		}
	}
}
