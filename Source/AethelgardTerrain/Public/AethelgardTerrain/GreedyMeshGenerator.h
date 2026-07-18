// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/VoxelMeshGenerator.h"
#include "GreedyMeshGenerator.generated.h"

UCLASS()
class AETHELGARDTERRAIN_API UGreedyMeshGenerator : public UVoxelMeshGenerator
{
    GENERATED_BODY()

public:
    virtual void GenerateMesh(
        const FChunkData& ChunkData,
        const TMap<FIntPoint, TSharedPtr<FChunkData>>& Neighbors,
        TMap<EBlockId, FMeshSectionData>& OutSections,
        float BlockScale = 100.0f) override;

private:
    void ProcessAxis(const FChunkData& CD,
        const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB,
        int32 Axis, int32 Sign, int32 LayerCount,
        TMap<EBlockId, FMeshSectionData>& Out, float Scale,
        int32 MinZ, int32 MaxZ, int32 EstimatedQuads);

    void AddQuad(FMeshSectionData& Out, EBlockId BlockType,
        int32 Axis, int32 Sign, int32 Layer,
        int32 U, int32 V, int32 W, int32 H, float Scale);

    EBlockId GetBlock(const FChunkData& CD,
        const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB,
        int32 X, int32 Y, int32 Z) const;

    FIntPoint GetChunkZBounds(const FChunkData& CD,
        const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB) const;
};
