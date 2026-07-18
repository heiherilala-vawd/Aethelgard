// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AethelgardTerrain/ChunkData.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "ChunkManagerComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkReady, const FIntPoint&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkRemoved, const FIntPoint&);

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class AETHELGARDTERRAIN_API UChunkManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UChunkManagerComponent();

    void SetWorldGenerator(UWorldGeneratorComponent* G) { Generator = G; }

    UPROPERTY(EditAnywhere, Category = "Chunks")
    int32 ViewDistance = 4;

    void UpdateCenter(const FIntPoint& CenterBlock);

    TSharedPtr<FChunkData> GetChunk(const FIntPoint& C) const;
    int32 GetChunkCount() const { return AllChunks.Num(); }
    int32 GetGenQueueNum() const { return GenQueue.Num(); }
    int32 GetMeshQueueNum() const { return MeshQueue.Num(); }
    EBlockId GetBlock(int32 BX, int32 BY, int32 BZ) const;
    bool SetBlock(int32 BX, int32 BY, int32 BZ, EBlockId Block);

    virtual void TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F) override;

    FOnChunkReady OnChunkReadyForMesh;
    FOnChunkRemoved OnChunkRemoved;

private:
    UPROPERTY()
    UWorldGeneratorComponent* Generator = nullptr;

    TMap<FIntPoint, TSharedPtr<FChunkData>> AllChunks;

    TArray<FIntPoint> GenQueue;
    TArray<FIntPoint> MeshQueue;

    int32 ActiveGenTasks = 0;
    static constexpr int32 MaxActiveGen = 6;

    void LaunchGen(FIntPoint C);
    void OnGenComplete(FIntPoint C, TSharedPtr<FChunkData> D);
    void TryMesh(FIntPoint C);
    bool AllNeighborsReady(FIntPoint C) const;
    void EnqueueGen(const FIntPoint& C);
    void EnqueueMesh(const FIntPoint& C);

    void DesiredCoords(const FIntPoint& Center, int32 R, TArray<FIntPoint>& Out) const;

    FIntPoint BlockToChunk(int32 BX, int32 BY) const
    {
        return FIntPoint(
            (BX >= 0 ? BX : BX - CHUNK_SIZE + 1) / CHUNK_SIZE,
            (BY >= 0 ? BY : BY - CHUNK_SIZE + 1) / CHUNK_SIZE
        );
    }
};
