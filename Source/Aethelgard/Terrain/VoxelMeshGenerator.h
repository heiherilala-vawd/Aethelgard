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
        Vertices.Empty();
        Triangles.Empty();
        Normals.Empty();
        UVs.Empty();
        Colors.Empty();
        Tangents.Empty();
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
        FMeshSectionData& OutMeshData,
        float BlockScale = 100.0f) PURE_VIRTUAL(UVoxelMeshGenerator::GenerateMesh, );
};
