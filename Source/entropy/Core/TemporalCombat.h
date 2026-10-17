// ENTROPÍA · Geometría de combate (Fase 3)
// Lógica pura y probada: ¿una bala que va de A a B toca la cápsula del jugador? ¿En cuánto tiempo pegaría?
// La usan la bala (para hacerte daño) y el dash (para la esquiva perfecta).

#pragma once

#include "CoreMinimal.h"

namespace TemporalCombat
{
	/**
	 * ¿El tramo A→B toca la cápsula vertical (Center, HalfHeight, Radius)? OutFraction = 0..1 del tramo donde la toca
	 * por primera vez (0 si A ya está dentro).
	 */
	ENTROPY_API bool SegmentHitsCapsule(const FVector& A, const FVector& B, const FVector& Center, float HalfHeight, float Radius, float& OutFraction);

	/** Segundos hasta que algo en Pos con velocidad Vel toca la cápsula; -1 si no la toca en MaxSeconds. */
	ENTROPY_API float TimeToHitCapsule(const FVector& Pos, const FVector& Vel, const FVector& Center, float HalfHeight, float Radius, float MaxSeconds);
}
