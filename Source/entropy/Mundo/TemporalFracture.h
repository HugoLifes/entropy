// ENTROPÍA · Fractura Voronoi (Fase 2)
// Lógica pura, sin mundo ni componentes: parte una caja en celdas convexas irregulares (estilo cristal roto).
// Cada celda es un poliedro convexo descrito por sus caras (polígonos con normal hacia afuera, en sentido antihorario
// visto desde fuera). Las caras "interiores" son los cortes nuevos: se pintan de otro color.

#pragma once

#include "CoreMinimal.h"

struct FFracturePolygon
{
	TArray<FVector> Points;
	/** true = cara que nació de un corte (el interior del objeto). */
	bool bInterior = false;
};

struct FFractureCell
{
	TArray<FFracturePolygon> Faces;
};

namespace TemporalFracture
{
	/** Caja centrada en el origen. */
	ENTROPY_API FFractureCell MakeBox(const FVector& HalfSize);

	/** Recorta la celda y conserva el lado Plane.PlaneDot(P) <= 0; agrega la tapa interior. False si no queda nada. */
	ENTROPY_API bool ClipCell(const FFractureCell& In, const FPlane& Plane, FFractureCell& Out);

	/** Una celda por semilla: la caja recortada por los bisectores con las demás semillas. */
	ENTROPY_API TArray<FFractureCell> VoronoiFracture(const FVector& HalfSize, const TArray<FVector>& Seeds);

	/** Semillas deterministas: la mitad cerca del impacto (pedazos chicos ahí), el resto repartidas. */
	ENTROPY_API TArray<FVector> MakeSeeds(const FVector& HalfSize, int32 Count, const FVector& LocalImpact, int32 RandomSeed);

	ENTROPY_API float CellVolume(const FFractureCell& Cell);
	ENTROPY_API FVector CellCentroid(const FFractureCell& Cell);

	/** Normal de la cara hacia afuera (método de Newell: robusto con puntos casi alineados). */
	ENTROPY_API FVector FaceNormal(const FFracturePolygon& Face);

	/** Triángulos en abanico de una cara con NumPoints puntos (desde Base), con la cara visible hacia afuera en Unreal. */
	ENTROPY_API void AppendFanTriangles(int32 Base, int32 NumPoints, TArray<int32>& OutTriangles);

	/** Vértices sin repetir (sirven para la colisión convexa). */
	ENTROPY_API TArray<FVector> CellVertices(const FFractureCell& Cell);
}
