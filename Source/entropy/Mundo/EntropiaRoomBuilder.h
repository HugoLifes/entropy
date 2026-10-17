// ENTROPÍA · Para construir salas con cubos del motor (Fase 3b)
// Las salas de prueba se arman en runtime con bloques (sin assets). Antes cada sala tenía su propia copia de AddBlock;
// las nuevas usan esta.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMeshComponent;

namespace EntropiaRooms
{
	/**
	 * Agrega un bloque (cubo escalado) a la sala, en coordenadas locales. Con colisión = BlockAll (WorldStatic): tapa la
	 * vista y se puede trepar. Sin traslape: la burbuja de Brecha no congela la sala.
	 */
	ENTROPY_API UStaticMeshComponent* AddBlock(AActor* Room, const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision = true);

	/** Piso y cuatro paredes: X de 0 a Length, Y de -HalfWidth a +HalfWidth. */
	ENTROPY_API void AddShell(AActor* Room, float Length, float HalfWidth, float WallHeight, const FLinearColor& FloorColor, const FLinearColor& WallColor);
}
