// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChunkData.h"
#include "ProceduralMeshComponent.h"
#include "VoxelMeshGenerator.generated.h"

struct FMeshSectionData
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    void Reset()
    {
        Vertices.SetNum(0, EAllowShrinking::No);
        Triangles.SetNum(0, EAllowShrinking::No);
        Normals.SetNum(0, EAllowShrinking::No);
        UVs.SetNum(0, EAllowShrinking::No);
        Colors.SetNum(0, EAllowShrinking::No);
        Tangents.SetNum(0, EAllowShrinking::No);
    }

    void ReserveEstimated(int32 NumQuads)
    {
        Vertices.Reserve(NumQuads * 4);
        Triangles.Reserve(NumQuads * 6);
        Normals.Reserve(NumQuads * 4);
        UVs.Reserve(NumQuads * 4);
        Colors.Reserve(NumQuads * 4);
        Tangents.Reserve(NumQuads * 4);
    }
};

UCLASS(Abstract)
class UVoxelMeshGenerator : public UObject
{
    GENERATED_BODY()

public:
    virtual void GenerateMesh(
        const FChunkData& ChunkData,
        const TMap<FIntPoint, TSharedPtr<FChunkData>>& Neighbors,
        TMap<EBlockId, FMeshSectionData>& OutSections,
        float BlockScale = 100.0f) PURE_VIRTUAL(UVoxelMeshGenerator::GenerateMesh, );
};
