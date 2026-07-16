// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/SaveSystem.h"
#include "Kismet/GameplayStatics.h"

void USaveSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USaveSystem::RecordModification(const FIntVector& WorldPos, EBlockId OldBlock, EBlockId NewBlock)
{
    FIntPoint ChunkCoord = WorldToChunkCoord(WorldPos.X, WorldPos.Y);
    int32 LX = WorldPos.X - ChunkCoord.X * CHUNK_SIZE;
    int32 LY = WorldPos.Y - ChunkCoord.Y * CHUNK_SIZE;

    FBlockChange Change;
    Change.LocalX = LX;
    Change.LocalY = LY;
    Change.LocalZ = WorldPos.Z;
    Change.OldBlockId = static_cast<uint8>(OldBlock);
    Change.NewBlockId = static_cast<uint8>(NewBlock);

    TArray<FBlockChange>& ChunkChanges = PendingChanges.FindOrAdd(ChunkCoord);

    bool bFound = false;
    for (int32 i = 0; i < ChunkChanges.Num(); i++)
    {
        FBlockChange& Ex = ChunkChanges[i];
        if (Ex.LocalX == LX && Ex.LocalY == LY && Ex.LocalZ == WorldPos.Z)
        {
            Ex.NewBlockId = Change.NewBlockId;
            if (Ex.OldBlockId == Change.NewBlockId)
            {
                ChunkChanges.RemoveAtSwap(i, 1, EAllowShrinking::No);
            }
            bFound = true;
            break;
        }
    }

    if (!bFound) ChunkChanges.Add(Change);

    OnBlockModified.Broadcast(WorldPos, OldBlock, NewBlock);
}

bool USaveSystem::SaveWorld(const FString& InSlotName, int32 WorldSeed)
{
    UWorldSaveData* SaveData = Cast<UWorldSaveData>(
        UGameplayStatics::CreateSaveGameObject(UWorldSaveData::StaticClass()));
    if (!SaveData) return false;

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
    if (!UGameplayStatics::DoesSaveGameExist(InSlotName, 0)) return false;

    UWorldSaveData* SaveData = Cast<UWorldSaveData>(
        UGameplayStatics::LoadGameFromSlot(InSlotName, 0));
    if (!SaveData) return false;

    OutWorldSeed = SaveData->WorldSeed;
    OutChunks = SaveData->ModifiedChunks;

    PendingChanges.Empty();
    for (const FChunkSaveData& ChunkData : OutChunks)
        PendingChanges.Add(ChunkData.Coord, ChunkData.Changes);

    return true;
}

bool USaveSystem::IsBlockModified(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
    FIntPoint ChunkCoord = WorldToChunkCoord(WorldX, WorldY);
    int32 LX = WorldX - ChunkCoord.X * CHUNK_SIZE;
    int32 LY = WorldY - ChunkCoord.Y * CHUNK_SIZE;

    const TArray<FBlockChange>* ChunkChanges = PendingChanges.Find(ChunkCoord);
    if (!ChunkChanges) return false;

    for (const FBlockChange& C : *ChunkChanges)
        if (C.LocalX == LX && C.LocalY == LY && C.LocalZ == WorldZ)
            return true;
    return false;
}

FIntPoint USaveSystem::WorldToChunkCoord(int32 X, int32 Y) const
{
    return FIntPoint(
        FMath::FloorToInt((float)X / CHUNK_SIZE),
        FMath::FloorToInt((float)Y / CHUNK_SIZE)
    );
}
