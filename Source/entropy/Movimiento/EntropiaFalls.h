// ENTROPÍA · Caídas (Fase 3b)
// Qué pasa al aterrizar según la altura de la caída y si ibas en movimiento. Lógica pura (probada).
//   < 4 m: normal · 4 a 9 m: ruedas si te mueves; si no, aterrizaje duro · > 9 m: aterrizaje duro y 10 de daño por metro extra.

#pragma once

#include "CoreMinimal.h"

enum class EEntropiaLanding : uint8
{
	Normal,
	Roll,
	Hard
};

namespace EntropiaFalls
{
	constexpr float RollFrom = 400.f;
	constexpr float DamageFrom = 900.f;
	constexpr float DamagePerMeter = 10.f;
	/** Velocidad horizontal mínima para rodar (cm/s). */
	constexpr float RollSpeed = 200.f;

	ENTROPY_API EEntropiaLanding Decide(float FallHeight, float HorizontalSpeed, float& OutDamage);
}
