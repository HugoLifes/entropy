// ENTROPÍA · Caídas

#include "EntropiaFalls.h"

namespace EntropiaFalls
{
	EEntropiaLanding Decide(float FallHeight, float HorizontalSpeed, float& OutDamage)
	{
		OutDamage = 0.f;
		if (FallHeight < RollFrom)
		{
			return EEntropiaLanding::Normal;
		}
		if (FallHeight > DamageFrom)
		{
			OutDamage = (FallHeight - DamageFrom) / 100.f * DamagePerMeter;
			return EEntropiaLanding::Hard;
		}
		return HorizontalSpeed > RollSpeed ? EEntropiaLanding::Roll : EEntropiaLanding::Hard;
	}
}
