// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/SaveSystem.h"
#include "Kismet/GameplayStatics.h"

void USaveSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USaveSystem::RecordModification(const FIntVector& WorldPos, EBlockId OldBlock, EBlockId NewBlock)
{
    FIntVector ChunkCoord = WorldToChunkCoord(WorldPos.X, WorldPos.Y, WorldPos.Z);
    int32 LX = WorldToLocal(WorldPos.X, ChunkCoord.X);
    int32 LY = WorldToLocal(WorldPos.Y, ChunkCoord.Y);
    int32 LZ = WorldToLocal(WorldPos.Z, ChunkCoord.Z);

    FBlockChange Change;
    Change.LocalPos = FIntVector(LX, LY, LZ);
    Change.OldBlockId = static_cast<uint8>(OldBlock);
    Change.NewBlockId = static_cast<uint8>(NewBlock);

    TArray<FBlockChange>& ChunkChanges = PendingChanges.FindOrAdd(ChunkCoord);

    bool bFound = false;
    for (int32 i = 0; i < ChunkChanges.Num(); i++)
    {
        FBlockChange& Existing = ChunkChanges[i];
        if (Existing.LocalPos == Change.LocalPos)
        {
            Existing.NewBlockId = Change.NewBlockId;
            if (Existing.OldBlockId == Change.NewBlockId)
            {
                ChunkChanges.RemoveAtSwap(i, 1, EAllowShrinking::No);
            }
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        ChunkChanges.Add(Change);
    }

    OnBlockModified.Broadcast(WorldPos, OldBlock, NewBlock);
}

bool USaveSystem::SaveWorld(const FString& InSlotName, int32 WorldSeed)
{
    UWorldSaveData* SaveData = Cast<UWorldSaveData>(
        UGameplayStatics::CreateSaveGameObject(UWorldSaveData::StaticClass())
    );

    if (!SaveData)
        return false;

    SaveData->WorldSeed = WorldSeed;
    SaveData->SlotName = InSlotName;
    SaveData->SaveTime = FDateTime::Now();

    for (const auto& Pair : PendingChanges)
    {
        if (Pair.Value.Num() > 0)
        {
            FChunkSaveData ChunkData;
            ChunkData.Coord = Pair.Key;
            ChunkData.Changes = Pair.Value;
            SaveData->ModifiedChunks.Add(ChunkData);
        }
    }

    return UGameplayStatics::SaveGameToSlot(SaveData, InSlotName, 0);
}

bool USaveSystem::LoadWorld(const FString& InSlotName, int32& OutWorldSeed, TArray<FChunkSaveData>& OutChunks)
{
    if (!UGameplayStatics::DoesSaveGameExist(InSlotName, 0))
        return false;

    UWorldSaveData* SaveData = Cast<UWorldSaveData>(
        UGameplayStatics::LoadGameFromSlot(InSlotName, 0)
    );

    if (!SaveData)
        return false;

    OutWorldSeed = SaveData->WorldSeed;
    OutChunks = SaveData->ModifiedChunks;

    PendingChanges.Empty();
    for (const FChunkSaveData& ChunkData : OutChunks)
    {
        PendingChanges.Add(ChunkData.Coord, ChunkData.Changes);
    }

    return true;
}

bool USaveSystem::IsBlockModified(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
    FIntVector ChunkCoord = WorldToChunkCoord(WorldX, WorldY, WorldZ);
    int32 LX = WorldToLocal(WorldX, ChunkCoord.X);
    int32 LY = WorldToLocal(WorldY, ChunkCoord.Y);
    int32 LZ = WorldToLocal(WorldZ, ChunkCoord.Z);

    const TArray<FBlockChange>* ChunkChanges = PendingChanges.Find(ChunkCoord);
    if (!ChunkChanges)
        return false;

    for (const FBlockChange& Change : *ChunkChanges)
    {
        if (Change.LocalPos == FIntVector(LX, LY, LZ))
            return true;
    }

    return false;
}

FIntVector USaveSystem::WorldToChunkCoord(int32 X, int32 Y, int32 Z) const
{
    return FIntVector(
        FMath::FloorToInt((float)X / CHUNK_SIZE),
        FMath::FloorToInt((float)Y / CHUNK_SIZE),
        FMath::FloorToInt((float)Z / CHUNK_SIZE)
    );
}

int32 USaveSystem::WorldToLocal(int32 WorldCoord, int32 ChunkCoord) const
{
    return WorldCoord - ChunkCoord * CHUNK_SIZE;
}
