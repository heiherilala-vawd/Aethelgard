// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/ChunkData.h"
#include "Terrain/VoxelMeshGenerator.h"
#include "ChunkActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class AVoxelChunkActor : public AActor
{
    GENERATED_BODY()

public:
    AVoxelChunkActor();

    void SetChunkData(TSharedPtr<FChunkData> InChunkData) { ChunkData = InChunkData; }
    TSharedPtr<FChunkData> GetChunkData() const { return ChunkData; }

    void UpdateMesh(UVoxelMeshGenerator* MeshGenerator, const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors);

    UPROPERTY()
    UProceduralMeshComponent* MeshComponent;

    UPROPERTY()
    UMaterialInterface* VertexColorMaterial;

private:
    TSharedPtr<FChunkData> ChunkData;

    UMaterialInterface* GetOrCreateMaterial();
};
