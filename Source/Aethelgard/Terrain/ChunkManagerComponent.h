// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "ChunkManagerComponent.generated.h"

class UVoxelMeshGenerator;
class UWorldGeneratorComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkReady, const FIntPoint&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkRemoved, const FIntPoint&);

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class UChunkManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UChunkManagerComponent();

    void SetWorldGenerator(UWorldGeneratorComponent* G) { Generator = G; }

    UPROPERTY(EditAnywhere, Category = "Chunks")
    int32 ViewDistance = 4;

    // All positions in BLOCK coordinates
    void UpdateCenter(const FIntPoint& CenterBlock);

    TSharedPtr<FChunkData> GetChunk(const FIntPoint& C) const;
    EBlockId GetBlock(int32 BX, int32 BY, int32 BZ) const;
    bool SetBlock(int32 BX, int32 BY, int32 BZ, EBlockId Block);

    virtual void TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F) override;

    FOnChunkReady OnChunkReadyForMesh;
    FOnChunkRemoved OnChunkRemoved;

private:
    UPROPERTY()
    UWorldGeneratorComponent* Generator = nullptr;

    TMap<FIntPoint, TSharedPtr<FChunkData>> AllChunks;
    TSet<FIntPoint> GeneratingChunks;
    TSet<FIntPoint> MeshReadyChunks;

    TArray<FIntPoint> PendingGeneration;
    TArray<FIntPoint> PendingRemoval;

    void GenerateOne(FIntPoint C);
    void RemoveOne(FIntPoint C);
    void TryEnqueueMesh(FIntPoint C);
    bool AllNeighborsGenerated(FIntPoint C) const;

    void AddDesiredChunks(const FIntPoint& Center, int32 Radius, TSet<FIntPoint>& Out);
    void GetChunksInRadius(const FIntPoint& Center, int32 Radius, TSet<FIntPoint>& Out);

    FIntPoint BlockToChunk(int32 BX, int32 BY) const
    {
        return FIntPoint(
            (BX >= 0 ? BX : BX - CHUNK_SIZE + 1) / CHUNK_SIZE,
            (BY >= 0 ? BY : BY - CHUNK_SIZE + 1) / CHUNK_SIZE
        );
    }
};
