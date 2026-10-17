// ENTROPÍA · Para construir salas con cubos del motor

#include "EntropiaRoomBuilder.h"
#include "TemporalFX.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

namespace EntropiaRooms
{
	UStaticMeshComponent* AddBlock(AActor* Room, const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollision)
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Room || !CubeMesh)
		{
			return nullptr;
		}
		UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(Room);
		Block->SetMobility(EComponentMobility::Movable);
		Block->SetStaticMesh(CubeMesh);
		Block->SetupAttachment(Room->GetRootComponent());
		Block->SetRelativeLocation(Center);
		Block->SetRelativeScale3D(Size / 100.f);
		Block->SetGenerateOverlapEvents(false);
		Block->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
		Block->RegisterComponent();
		Room->AddInstanceComponent(Block);
		TemporalFX::ApplyToAllSlots(Block, TemporalFX::MakeSolid(Room, Color));
		return Block;
	}

	void AddShell(AActor* Room, float Length, float HalfWidth, float WallHeight, const FLinearColor& FloorColor, const FLinearColor& WallColor)
	{
		constexpr float ShellThick = 50.f;
		const float WallZ = WallHeight * 0.5f;
		AddBlock(Room, FVector(Length * 0.5f, 0.f, -ShellThick * 0.5f), FVector(Length, HalfWidth * 2.f, ShellThick), FloorColor);
		for (const float Side : { -1.f, 1.f })
		{
			AddBlock(Room, FVector(Length * 0.5f, Side * (HalfWidth + ShellThick * 0.5f), WallZ), FVector(Length, ShellThick, WallHeight), WallColor);
		}
		for (const float X : { -ShellThick * 0.5f, Length + ShellThick * 0.5f })
		{
			AddBlock(Room, FVector(X, 0.f, WallZ), FVector(ShellThick, HalfWidth * 2.f + ShellThick * 2.f, WallHeight), WallColor);
		}
	}
}
