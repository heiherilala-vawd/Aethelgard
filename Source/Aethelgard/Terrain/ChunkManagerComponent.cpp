// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/WorldGeneratorComponent.h"

UChunkManagerComponent::UChunkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UChunkManagerComponent::UpdateCenter(const FIntPoint& CenterBlock)
{
    TSet<FIntPoint> Desired;
    for (int32 DY = -ViewDistance; DY <= ViewDistance; DY++)
        for (int32 DX = -ViewDistance; DX <= ViewDistance; DX++)
        {
            if (DX * DX + DY * DY <= ViewDistance * ViewDistance)
                Desired.Add(FIntPoint(CenterBlock.X / CHUNK_SIZE + DX, CenterBlock.Y / CHUNK_SIZE + DY));
        }

    TArray<FIntPoint> ToRemove;
    for (const FIntPoint& C : ActiveChunks)
        if (!Desired.Contains(C))
            ToRemove.Add(C);

    for (const FIntPoint& C : ToRemove)
    {
        PendingQueue.Enqueue({C, false});
        ActiveChunks.Remove(C);
    }

    for (const FIntPoint& C : Desired)
        if (!ActiveChunks.Contains(C))
        {
            PendingQueue.Enqueue({C, true});
            ActiveChunks.Add(C);
        }
}

void UChunkManagerComponent::TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F)
{
    Super::TickComponent(DT, T, F);

    for (int32 i = 0; i < 3; i++)
    {
        FPendingChunk P;
        if (!PendingQueue.Dequeue(P)) break;

        if (P.bGenerate)
            GenerateAndMesh(P.Coord);
        else
            RemoveChunk(P.Coord);
    }
}

void UChunkManagerComponent::GenerateAndMesh(const FIntPoint& C)
{
    if (!Generator || !Mesher) return;

    TSharedPtr<FChunkData>* Existing = AllChunks.Find(C);
    TSharedPtr<FChunkData> Data;
    if (Existing && Existing->IsValid())
        Data = *Existing;
    else
    {
        Data = MakeShared<FChunkData>();
        Data->Initialize(C);
        AllChunks.Add(C, Data);
    }

    if (!Data->bIsGenerated)
        Generator->GenerateChunk(*Data);

    OnChunkReadyForMesh.Broadcast(C);
}

void UChunkManagerComponent::RemoveChunk(const FIntPoint& C)
{
    AllChunks.Remove(C);
    OnChunkRemoved.Broadcast(C);
}

TSharedPtr<FChunkData> UChunkManagerComponent::GetChunk(const FIntPoint& C) const
{
    const auto* Found = AllChunks.Find(C);
    return Found ? *Found : nullptr;
}

EBlockId UChunkManagerComponent::GetBlock(int32 BX, int32 BY, int32 BZ) const
{
    FIntPoint C = BlockToChunk(BX, BY);
    auto* D = AllChunks.Find(C);
    if (!D || !D->IsValid() || !(*D)->bIsGenerated) return EBlockId::Air;
    return (*D)->GetBlock(BX - C.X * CHUNK_SIZE, BY - C.Y * CHUNK_SIZE, BZ);
}

bool UChunkManagerComponent::SetBlock(int32 BX, int32 BY, int32 BZ, EBlockId Block)
{
    FIntPoint C = BlockToChunk(BX, BY);
    auto* D = AllChunks.Find(C);
    if (!D || !D->IsValid()) return false;

    (*D)->SetBlock(BX - C.X * CHUNK_SIZE, BY - C.Y * CHUNK_SIZE, BZ, Block);
    OnChunkReadyForMesh.Broadcast(C);
    return true;
}
