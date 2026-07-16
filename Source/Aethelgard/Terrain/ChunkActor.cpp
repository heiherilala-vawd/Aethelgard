// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/ChunkActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"

AVoxelChunkActor::AVoxelChunkActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetCastShadow(false);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->bUseComplexAsSimpleCollision = false;
}

void AVoxelChunkActor::UpdateMesh(
    UVoxelMeshGenerator* MeshGenerator,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors)
{
    if (!MeshGenerator || !ChunkData.IsValid())
        return;

    FMeshSectionData MeshData;
    MeshGenerator->GenerateMesh(*ChunkData, Neighbors, MeshData);

    MeshComponent->ClearMeshSection(0);

    if (MeshData.Vertices.Num() == 0)
        return;

    TArray<FVector> Normals = MeshData.Normals;
    if (Normals.Num() == 0)
    {
        Normals.Init(FVector::UpVector, MeshData.Vertices.Num());
    }

    MeshComponent->CreateMeshSection(
        0,
        MeshData.Vertices,
        MeshData.Triangles,
        Normals,
        MeshData.UVs,
        MeshData.Colors,
        MeshData.Tangents,
        true
    );

    if (MeshComponent->GetNumMaterials() == 0)
    {
        MeshComponent->SetMaterial(0, GetOrCreateMaterial());
    }
}

UMaterialInterface* AVoxelChunkActor::GetOrCreateMaterial()
{
    if (!VertexColorMaterial)
    {
        VertexColorMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    }
    return VertexColorMaterial;
}
