// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Terrain/VoxelMeshGenerator.h"
#include "GreedyMeshGenerator.generated.h"

UCLASS()
class UGreedyMeshGenerator : public UVoxelMeshGenerator
{
    GENERATED_BODY()

public:
    virtual void GenerateMesh(
        const FChunkData& ChunkData,
        const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
        FMeshSectionData& OutMeshData) override;

private:
    void ProcessSlice(
        const FChunkData& ChunkData,
        const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
        int32 Axis,
        int32 Direction,
        int32 Layer,
        FMeshSectionData& OutMeshData);

    EBlockId GetBlockAt(
        const FChunkData& CenterChunk,
        const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
        int32 X, int32 Y, int32 Z) const;

    void AddQuad(
        FMeshSectionData& MeshData,
        FColor Color,
        int32 Axis,
        int32 Direction,
        int32 Layer,
        int32 U, int32 V,
        int32 Width, int32 Height);
};
