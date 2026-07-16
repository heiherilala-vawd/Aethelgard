// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "ChunkManagerComponent.generated.h"

class AVoxelChunkActor;
class UVoxelMeshGenerator;
class UWorldGeneratorComponent;

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class UChunkManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UChunkManagerComponent();

    void SetWorldGenerator(UWorldGeneratorComponent* InGenerator) { WorldGenerator = InGenerator; }
    void SetMeshGenerator(UVoxelMeshGenerator* InGenerator) { MeshGenerator = InGenerator; }

    UPROPERTY(EditAnywhere, Category = "Chunks")
    int32 ViewDistance = 8;

    void UpdatePlayerPosition(const FVector& WorldPosition);

    TSharedPtr<FChunkData> GetOrCreateChunkData(const FIntVector& Coord);

    EBlockId GetBlock(int32 WorldX, int32 WorldY, int32 WorldZ) const;
    bool SetBlock(int32 WorldX, int32 WorldY, int32 WorldZ, EBlockId Block);

    void RebuildChunkMesh(const FIntVector& Coord);

    const TMap<FIntVector, TSharedPtr<FChunkData>>& GetAllChunkData() const { return AllChunks; }

private:
    UPROPERTY()
    UWorldGeneratorComponent* WorldGenerator = nullptr;

    UPROPERTY()
    UVoxelMeshGenerator* MeshGenerator = nullptr;

    TMap<FIntVector, TSharedPtr<FChunkData>> AllChunks;
    TMap<FIntVector, TObjectPtr<AVoxelChunkActor>> ActiveChunkActors;

    void LoadChunk(const FIntVector& Coord);
    void UnloadChunk(const FIntVector& Coord);
    void RefreshNeighborMeshes(const FIntVector& Coord);

    FIntVector WorldToChunkCoord(int32 WorldX, int32 WorldY, int32 WorldZ) const;
};
