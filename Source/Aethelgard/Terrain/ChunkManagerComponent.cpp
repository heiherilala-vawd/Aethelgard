// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/WorldGeneratorComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogChunkMgr, Log, All);

UChunkManagerComponent::UChunkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UChunkManagerComponent::UpdateCenter(const FIntPoint& CenterBlock)
{
    int32 GenRadius = ViewDistance + 1;
    int32 MeshRadius = ViewDistance;

    TSet<FIntPoint> DesiredGen;
    TSet<FIntPoint> DesiredMesh;
    GetChunksInRadius(CenterBlock, GenRadius, DesiredGen);
    GetChunksInRadius(CenterBlock, MeshRadius, DesiredMesh);

    // Remove chunks that are out of range of generation
    TArray<FIntPoint> ToRemove;
    for (const FIntPoint& C : GeneratingChunks)
        if (!DesiredGen.Contains(C))
            ToRemove.Add(C);

    // Also remove mesh-only out-of-range if they were somehow tracked
    for (const FIntPoint& C : MeshReadyChunks)
        if (!DesiredMesh.Contains(C))
            ToRemove.AddUnique(C);

    for (const FIntPoint& C : ToRemove)
    {
        PendingRemoval.Add(C);
        GeneratingChunks.Remove(C);
        MeshReadyChunks.Remove(C);
    }

    // Add new chunks for generation
    for (const FIntPoint& C : DesiredGen)
        if (!GeneratingChunks.Contains(C) && !AllChunks.Contains(C))
        {
            GeneratingChunks.Add(C);
            TSharedPtr<FChunkData> Data = MakeShared<FChunkData>();
            Data->Initialize(C);
            AllChunks.Add(C, Data);
            PendingGeneration.Add(C);
        }
}

void UChunkManagerComponent::TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F)
{
    Super::TickComponent(DT, T, F);

    // Process removals (1 per frame, immediate)
    if (PendingRemoval.Num() > 0)
    {
        FIntPoint C = PendingRemoval.Pop();
        AllChunks.Remove(C);
        OnChunkRemoved.Broadcast(C);
    }

    // Generate blocks (3 per frame)
    for (int32 i = 0; i < 3 && PendingGeneration.Num() > 0; i++)
    {
        FIntPoint C = PendingGeneration.Pop();
        GenerateOne(C);
    }

    // Mesh ready chunks (2 per frame)
    TArray<FIntPoint> ToMesh;
    for (const FIntPoint& C : MeshReadyChunks)
        if (ToMesh.Num() < 2)
            ToMesh.Add(C);

    for (const FIntPoint& C : ToMesh)
    {
        MeshReadyChunks.Remove(C);
        OnChunkReadyForMesh.Broadcast(C);
    }
}

void UChunkManagerComponent::GenerateOne(FIntPoint C)
{
    if (!Generator) return;

    TSharedPtr<FChunkData>* Data = AllChunks.Find(C);
    if (!Data || !Data->IsValid()) return;

    if (!(*Data)->bIsGenerated)
        Generator->GenerateChunk(*(*Data));

    GeneratingChunks.Remove(C);
    TryEnqueueMesh(C);

    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
            if (DX != 0 || DY != 0)
                TryEnqueueMesh(FIntPoint(C.X + DX, C.Y + DY));
}

void UChunkManagerComponent::RemoveOne(FIntPoint C)
{
    AllChunks.Remove(C);
    OnChunkRemoved.Broadcast(C);
}

void UChunkManagerComponent::TryEnqueueMesh(FIntPoint C)
{
    if (AllNeighborsGenerated(C))
    {
        UE_LOG(LogChunkMgr, Log, TEXT("Mesh ready (%d, %d)"), C.X, C.Y);
        MeshReadyChunks.Add(C);
    }
    else
    {
        UE_LOG(LogChunkMgr, Verbose, TEXT("Mesh waiting (%d, %d) - neighbors not ready"), C.X, C.Y);
    }
}

bool UChunkManagerComponent::AllNeighborsGenerated(FIntPoint C) const
{
    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
        {
            if (DX == 0 && DY == 0) continue;
            FIntPoint N(C.X + DX, C.Y + DY);
            const TSharedPtr<FChunkData>* D = AllChunks.Find(N);
            if (!D || !D->IsValid() || !(*D)->bIsGenerated)
                return false;
        }
    return true;
}

void UChunkManagerComponent::GetChunksInRadius(const FIntPoint& Center, int32 Radius, TSet<FIntPoint>& Out)
{
    FIntPoint CC = BlockToChunk(Center.X, Center.Y);
    for (int32 DY = -Radius; DY <= Radius; DY++)
        for (int32 DX = -Radius; DX <= Radius; DX++)
            if (DX * DX + DY * DY <= Radius * Radius)
                Out.Add(FIntPoint(CC.X + DX, CC.Y + DY));
}

void UChunkManagerComponent::AddDesiredChunks(const FIntPoint& Center, int32 Radius, TSet<FIntPoint>& Out)
{
    for (int32 DY = -Radius; DY <= Radius; DY++)
        for (int32 DX = -Radius; DX <= Radius; DX++)
            if (DX * DX + DY * DY <= Radius * Radius)
                Out.Add(FIntPoint(Center.X + DX, Center.Y + DY));
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

    // Re-mesh this chunk and all neighbors in mesh radius
    MeshReadyChunks.Add(C);
    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
            if (DX != 0 || DY != 0)
                MeshReadyChunks.Add(FIntPoint(C.X + DX, C.Y + DY));

    return true;
}
