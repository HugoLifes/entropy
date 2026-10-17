// ENTROPÍA · Parkour: la parte que decide (Fase 3b)
// Lógica pura y probada: qué movimiento hacer según el obstáculo que tienes enfrente, y por dónde pasa el cuerpo.
//   Dash vault: barda de 50-130 cm de alto y hasta 70 cm de fondo, con piso del otro lado.
//   Gato (kong / cat pass): corriendo, mismo alto, 70-220 cm de fondo.
//   Subir: hasta 130 cm (cuando no se puede pasar por encima). Trepar: 130-230 cm.
//   Correr por la pared: 230-340 cm, solo corriendo.
// El componente (EntropiaParkourComponent) mide el obstáculo con trazos y mueve al personaje por PathPoint.

#pragma once

#include "CoreMinimal.h"

/** Lo que se midió enfrente del jugador. Alturas desde los pies; distancias en cm. */
struct FEntropiaObstacle
{
	bool bHit = false;
	/** De la orilla de la cápsula a la cara del obstáculo. */
	float Distance = 0.f;
	/** Altura del borde sobre los pies. */
	float Height = 0.f;
	/** Fondo del obstáculo (999 = no se le encontró el final: plataforma o muro). */
	float Depth = 999.f;
	/** Cabe la cápsula de pie arriba del borde. */
	bool bTopClear = false;
	/** Hay piso y espacio del otro lado para pasar por encima. */
	bool bLandingClear = false;
};

enum class EEntropiaTraversal : uint8
{
	None,
	DashVault,
	Kong,
	StepUp,
	Climb,
	WallRun,
	/** Subir desde colgado de una orilla. */
	HangClimb
};

namespace EntropiaParkour
{
	constexpr float MinHeight = 50.f;
	constexpr float VaultMaxHeight = 130.f;
	constexpr float VaultMaxDepth = 70.f;
	constexpr float KongMaxDepth = 220.f;
	constexpr float ClimbMaxHeight = 230.f;
	constexpr float WallRunMaxHeight = 340.f;
	/** Qué tan lejos puede estar el obstáculo para empezar. */
	constexpr float MaxDistance = 150.f;
	/** Separación entre el centro de la cápsula y la pared al trepar (radio + margen). */
	constexpr float WallGap = 47.f;
	/** Cuánto pasa la cadera por encima del borde al saltarlo. */
	constexpr float VaultClearance = 55.f;
	/** Una orilla se puede agarrar si está entre estas alturas sobre los pies. */
	constexpr float LedgeMinHeight = 120.f;
	constexpr float LedgeMaxHeight = 230.f;
	/** Colgado: el centro de la cápsula queda esto por debajo del borde (los brazos arriba). */
	constexpr float HangDrop = 120.f;

	ENTROPY_API EEntropiaTraversal Classify(const FEntropiaObstacle& Obstacle, bool bSprinting);

	/** ¿Alcanzan las manos una orilla a esta altura sobre los pies? */
	ENTROPY_API bool CanGrabLedge(float EdgeHeightAboveFeet);

	/** Posición del centro de la cápsula en Alpha (0 = Start, 1 = End). Edge = punto de la cara del obstáculo a la altura del borde. */
	ENTROPY_API FVector PathPoint(EEntropiaTraversal Kind, const FVector& Start, const FVector& Edge, const FVector& End, float Alpha);

	/** Segundos que dura cada movimiento. */
	ENTROPY_API float Duration(EEntropiaTraversal Kind);

	ENTROPY_API const TCHAR* Name(EEntropiaTraversal Kind);
}
