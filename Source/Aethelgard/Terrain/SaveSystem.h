// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Terrain/BlockRegistry.h"
#include "Terrain/ChunkData.h"
#include "SaveSystem.generated.h"

USTRUCT()
struct FBlockChange
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
struct FChunkSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FIntPoint Coord;

    UPROPERTY()
    TArray<FBlockChange> Changes;
};

UCLASS()
class UWorldSaveData : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 WorldSeed = 0;

    UPROPERTY()
    TArray<FChunkSaveData> ModifiedChunks;

    UPROPERTY()
    FString SlotName;

    UPROPERTY()
    FDateTime SaveTime;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBlockModified, const FIntVector&, EBlockId, EBlockId);

UCLASS()
class USaveSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RecordModification(const FIntVector& WorldPos, EBlockId OldBlock, EBlockId NewBlock);
    bool SaveWorld(const FString& InSlotName, int32 WorldSeed);
    bool LoadWorld(const FString& InSlotName, int32& OutWorldSeed, TArray<FChunkSaveData>& OutChunks);

    FOnBlockModified OnBlockModified;
    bool IsBlockModified(int32 WorldX, int32 WorldY, int32 WorldZ) const;

private:
    TMap<FIntPoint, TArray<FBlockChange>> PendingChanges;

    FIntPoint WorldToChunkCoord(int32 X, int32 Y) const;
};
