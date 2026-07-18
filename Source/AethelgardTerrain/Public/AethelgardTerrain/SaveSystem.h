// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AethelgardTerrain/BlockRegistry.h"
#include "AethelgardTerrain/ChunkData.h"
#include "SaveSystem.generated.h"

USTRUCT()
struct AETHELGARDTERRAIN_API FBlockChange
{
    GENERATED_BODY()

    UPROPERTY()
    int32 LocalX = 0;

    UPROPERTY()
    int32 LocalY = 0;

    UPROPERTY()
    int32 LocalZ = 0;

    UPROPERTY()
    uint8 OldBlockId = 0;

    UPROPERTY()
    uint8 NewBlockId = 0;
};

USTRUCT()
struct AETHELGARDTERRAIN_API FChunkSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FIntPoint Coord;

    UPROPERTY()
    TArray<FBlockChange> Changes;
};

USTRUCT()
struct AETHELGARDTERRAIN_API FInventorySlotSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    uint8 BlockId = 0;

    UPROPERTY()
    int32 Quantity = 0;
};

UCLASS()
class AETHELGARDTERRAIN_API UWorldSaveData : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 WorldSeed = 0;

    UPROPERTY()
    TArray<FChunkSaveData> ModifiedChunks;

    UPROPERTY()
    TArray<FInventorySlotSaveData> SavedInventory;

    UPROPERTY()
    FString SlotName;

    UPROPERTY()
    FDateTime SaveTime;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBlockModified, const FIntVector&, EBlockId, EBlockId);

UCLASS()
class AETHELGARDTERRAIN_API USaveSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RecordModification(const FIntVector& WorldPos, EBlockId OldBlock, EBlockId NewBlock);
    bool SaveWorld(const FString& InSlotName, int32 WorldSeed, const TArray<FInventorySlotSaveData>& Inventory);
    bool LoadWorld(const FString& InSlotName, int32& OutWorldSeed, TArray<FChunkSaveData>& OutChunks, TArray<FInventorySlotSaveData>& OutInventory);

    FOnBlockModified OnBlockModified;
    bool IsBlockModified(int32 WorldX, int32 WorldY, int32 WorldZ) const;

private:
    TMap<FIntPoint, TArray<FBlockChange>> PendingChanges;

    FIntPoint WorldToChunkCoord(int32 X, int32 Y) const;
};
