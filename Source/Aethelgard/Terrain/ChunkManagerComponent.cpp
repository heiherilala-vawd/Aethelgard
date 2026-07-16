// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/ChunkActor.h"
#include "Terrain/WorldGeneratorComponent.h"
#include "Terrain/GreedyMeshGenerator.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogChunkManager, Log, All);

UChunkManagerComponent::UChunkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UChunkManagerComponent::UpdatePlayerPosition(const FVector& WorldPosition)
{
    FIntVector PlayerChunk(
        FMath::FloorToInt(WorldPosition.X / CHUNK_SIZE),
        FMath::FloorToInt(WorldPosition.Y / CHUNK_SIZE),
        FMath::FloorToInt(WorldPosition.Z / CHUNK_SIZE)
    );

    TSet<FIntVector> DesiredChunks;
    for (int32 DZ = -ViewDistance; DZ <= ViewDistance; DZ++)
    {
        for (int32 DY = -ViewDistance; DY <= ViewDistance; DY++)
        {
            for (int32 DX = -ViewDistance; DX <= ViewDistance; DX++)
            {
                int32 DistSq = DX * DX + DY * DY + DZ * DZ;
                if (DistSq <= ViewDistance * ViewDistance)
                {
                    DesiredChunks.Add(FIntVector(
                        PlayerChunk.X + DX,
                        PlayerChunk.Y + DY,
                        PlayerChunk.Z + DZ
                    ));
                }
            }
        }
    }

    TArray<FIntVector> ToUnload;
    for (const auto& Pair : ActiveChunkActors)
    {
        if (!DesiredChunks.Contains(Pair.Key))
        {
            ToUnload.Add(Pair.Key);
        }
    }

    for (const FIntVector& Coord : ToUnload)
    {
        UnloadChunk(Coord);
    }

    for (const FIntVector& Coord : DesiredChunks)
    {
        if (!ActiveChunkActors.Contains(Coord))
        {
            LoadChunk(Coord);
        }
    }
}

void UChunkManagerComponent::LoadChunk(const FIntVector& Coord)
{
    if (!WorldGenerator || !MeshGenerator)
        return;

    TSharedPtr<FChunkData> ChunkData = GetOrCreateChunkData(Coord);
    if (!ChunkData->bIsGenerated)
    {
        WorldGenerator->GenerateChunk(*ChunkData);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    FVector WorldPos(
        (float)(Coord.X * CHUNK_SIZE),
        (float)(Coord.Y * CHUNK_SIZE),
        (float)(Coord.Z * CHUNK_SIZE)
    );

    AVoxelChunkActor* ChunkActor = GetWorld()->SpawnActor<AVoxelChunkActor>(WorldPos, FRotator::ZeroRotator, SpawnParams);
    if (ChunkActor)
    {
        UE_LOG(LogChunkManager, Log, TEXT("  Loaded chunk (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);
        ChunkActor->SetChunkData(ChunkData);
        ActiveChunkActors.Add(Coord, ChunkActor);

        TMap<FIntVector, TSharedPtr<FChunkData>> Neighbors;
        for (int32 DZ = -1; DZ <= 1; DZ++)
        {
            for (int32 DY = -1; DY <= 1; DY++)
            {
                for (int32 DX = -1; DX <= 1; DX++)
                {
                    if (DX == 0 && DY == 0 && DZ == 0)
                        continue;
                    FIntVector NCoord(Coord.X + DX, Coord.Y + DY, Coord.Z + DZ);
                    TSharedPtr<FChunkData>* NP = AllChunks.Find(NCoord);
                    if (NP && NP->IsValid())
                    {
                        Neighbors.Add(NCoord, *NP);
                    }
                }
            }
        }

        ChunkActor->UpdateMesh(MeshGenerator, Neighbors);
    }
}

void UChunkManagerComponent::UnloadChunk(const FIntVector& Coord)
{
    TObjectPtr<AVoxelChunkActor>* ActorPtr = ActiveChunkActors.Find(Coord);
    if (ActorPtr && *ActorPtr)
    {
        (*ActorPtr)->Destroy();
    }
    ActiveChunkActors.Remove(Coord);
}

TSharedPtr<FChunkData> UChunkManagerComponent::GetOrCreateChunkData(const FIntVector& Coord)
{
    TSharedPtr<FChunkData>* Existing = AllChunks.Find(Coord);
    if (Existing)
        return *Existing;

    TSharedPtr<FChunkData> NewData = MakeShared<FChunkData>();
    NewData->Initialize(Coord);
    AllChunks.Add(Coord, NewData);
    return NewData;
}

EBlockId UChunkManagerComponent::GetBlock(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
    FIntVector Coord = WorldToChunkCoord(WorldX, WorldY, WorldZ);
    const TSharedPtr<FChunkData>* Data = AllChunks.Find(Coord);
    if (!Data || !Data->IsValid() || !(*Data)->bIsGenerated)
        return EBlockId::Air;

    int32 LX = WorldX - Coord.X * CHUNK_SIZE;
    int32 LY = WorldY - Coord.Y * CHUNK_SIZE;
    int32 LZ = WorldZ - Coord.Z * CHUNK_SIZE;

    return (*Data)->GetBlock(LX, LY, LZ);
}

bool UChunkManagerComponent::SetBlock(int32 WorldX, int32 WorldY, int32 WorldZ, EBlockId Block)
{
    FIntVector Coord = WorldToChunkCoord(WorldX, WorldY, WorldZ);
    TSharedPtr<FChunkData>* Data = AllChunks.Find(Coord);
    if (!Data || !Data->IsValid())
        return false;

    int32 LX = WorldX - Coord.X * CHUNK_SIZE;
    int32 LY = WorldY - Coord.Y * CHUNK_SIZE;
    int32 LZ = WorldZ - Coord.Z * CHUNK_SIZE;

    (*Data)->SetBlock(LX, LY, LZ, Block);

    RebuildChunkMesh(Coord);
    RefreshNeighborMeshes(Coord);

    return true;
}

void UChunkManagerComponent::RebuildChunkMesh(const FIntVector& Coord)
{
    TObjectPtr<AVoxelChunkActor>* ActorPtr = ActiveChunkActors.Find(Coord);
    if (!ActorPtr || !*ActorPtr || !MeshGenerator)
        return;

    TMap<FIntVector, TSharedPtr<FChunkData>> Neighbors;
    for (int32 DZ = -1; DZ <= 1; DZ++)
    {
        for (int32 DY = -1; DY <= 1; DY++)
        {
            for (int32 DX = -1; DX <= 1; DX++)
            {
                if (DX == 0 && DY == 0 && DZ == 0)
                    continue;
                FIntVector NCoord(Coord.X + DX, Coord.Y + DY, Coord.Z + DZ);
                TSharedPtr<FChunkData>* NP = AllChunks.Find(NCoord);
                if (NP && NP->IsValid())
                {
                    Neighbors.Add(NCoord, *NP);
                }
            }
        }
    }

    (*ActorPtr)->UpdateMesh(MeshGenerator, Neighbors);
}

void UChunkManagerComponent::RefreshNeighborMeshes(const FIntVector& Coord)
{
    for (int32 DZ = -1; DZ <= 1; DZ++)
    {
        for (int32 DY = -1; DY <= 1; DY++)
        {
            for (int32 DX = -1; DX <= 1; DX++)
            {
                if (DX == 0 && DY == 0 && DZ == 0)
                    continue;
                FIntVector NCoord(Coord.X + DX, Coord.Y + DY, Coord.Z + DZ);
                if (ActiveChunkActors.Contains(NCoord))
                {
                    RebuildChunkMesh(NCoord);
                }
            }
        }
    }
}

FIntVector UChunkManagerComponent::WorldToChunkCoord(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
    return FIntVector(
        FMath::FloorToInt((float)WorldX / CHUNK_SIZE),
        FMath::FloorToInt((float)WorldY / CHUNK_SIZE),
        FMath::FloorToInt((float)WorldZ / CHUNK_SIZE)
    );
}
