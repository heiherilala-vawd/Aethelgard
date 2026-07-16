// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Terrain/VoxelMeshGenerator.h"
#include "SurfaceNetsMeshGenerator.generated.h"

UCLASS()
class USurfaceNetsMeshGenerator : public UVoxelMeshGenerator
{
    GENERATED_BODY()

public:
    virtual void GenerateMesh(
        const FChunkData& ChunkData,
        const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
        FMeshSectionData& OutMeshData) override;
};
