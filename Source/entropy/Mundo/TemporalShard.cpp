// ENTROPÍA · Pedazo irregular

#include "TemporalShard.h"
#include "TemporalFracture.h"
#include "TemporalRecorderComponent.h"

#include "ProceduralMeshComponent.h"

ATemporalShard::ATemporalShard()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	// Colisión simple (convexa) para poder simular física; la compleja no sirve con cuerpos dinámicos.
	Mesh->bUseComplexAsSimpleCollision = false;
	Mesh->SetCastShadow(true);
	SetRootComponent(Mesh);

	Recorder = CreateDefaultSubobject<UTemporalRecorderComponent>(TEXT("Recorder"));
}

void ATemporalShard::Build(const FFractureCell& Cell, const FVector& CellCentroid, UMaterialInterface* OuterMaterial, UMaterialInterface* InnerMaterial,
	float MemorySeconds, float Shrink)
{
	// Una sección por tipo de cara: 0 = afuera, 1 = corte. Vértices por cara (normales planas, se ve "facetado").
	TArray<FVector> Vertices[2];
	TArray<int32> Triangles[2];
	TArray<FVector> Normals[2];

	for (const FFracturePolygon& Face : Cell.Faces)
	{
		if (Face.Points.Num() < 3)
		{
			continue;
		}
		const int32 Section = Face.bInterior ? 1 : 0;
		const FVector Normal = TemporalFracture::FaceNormal(Face);
		const int32 Base = Vertices[Section].Num();
		for (const FVector& P : Face.Points)
		{
			Vertices[Section].Add((P - CellCentroid) * Shrink);
			Normals[Section].Add(Normal);
		}
		// Abanico desde el primer punto (la cara es convexa), con la cara visible hacia afuera.
		TemporalFracture::AppendFanTriangles(Base, Face.Points.Num(), Triangles[Section]);
	}

	const TArray<FVector2D> NoUV;
	const TArray<FLinearColor> NoColors;
	const TArray<FProcMeshTangent> NoTangents;
	for (int32 Section = 0; Section < 2; ++Section)
	{
		if (Triangles[Section].Num() > 0)
		{
			Mesh->CreateMeshSection_LinearColor(Section, Vertices[Section], Triangles[Section], Normals[Section], NoUV, NoColors, NoTangents, /*bCreateCollision=*/false);
		}
	}
	Mesh->SetMaterial(0, OuterMaterial);
	Mesh->SetMaterial(1, InnerMaterial ? InnerMaterial : OuterMaterial);

	// Colisión: el casco convexo de la celda (ya es convexa), encogido igual que la malla.
	TArray<FVector> Hull = TemporalFracture::CellVertices(Cell);
	for (FVector& P : Hull)
	{
		P = (P - CellCentroid) * Shrink;
	}
	// Una sola llamada: la colisión convexa se calcula una vez por pedazo.
	Mesh->SetCollisionConvexMeshes({ Hull });
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);

	Recorder->MaxRecordSeconds = MemorySeconds;
}
