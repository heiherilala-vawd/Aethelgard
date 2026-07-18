// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/ChunkManagerComponent.h"
#include "Async/Async.h"

UChunkManagerComponent::UChunkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UChunkManagerComponent::UpdateCenter(const FIntPoint& CenterBlock)
{
    TArray<FIntPoint> Desired, DesiredMesh;
    DesiredCoords(CenterBlock, ViewDistance + 1, Desired);

    TSet<FIntPoint> Keep;
    for (const FIntPoint& C : Desired)
        Keep.Add(C);

    TArray<FIntPoint> ToRemove;
    ToRemove.Reserve(64);
    for (const auto& KV : AllChunks)
        if (!Keep.Contains(KV.Key))
            ToRemove.Add(KV.Key);

    for (const FIntPoint& C : ToRemove)
    {
        AllChunks.Remove(C);
        OnChunkRemoved.Broadcast(C);
    }

    for (const FIntPoint& C : Desired)
    {
        if (AllChunks.Contains(C)) continue;
        TSharedPtr<FChunkData> D = MakeShared<FChunkData>();
        D->Initialize(C);
        AllChunks.Add(C, D);
        EnqueueGen(C);
    }
}

void UChunkManagerComponent::TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F)
{
    Super::TickComponent(DT, T, F);

    if (ActiveGenTasks < MaxActiveGen && GenQueue.Num() > 0)
    {
        FIntPoint C = GenQueue.Last();
        GenQueue.RemoveAt(GenQueue.Num() - 1);
        LaunchGen(C);
    }

    for (int32 i = 0; i < 3 && MeshQueue.Num() > 0; i++)
    {
        FIntPoint C = MeshQueue.Last();
        MeshQueue.RemoveAt(MeshQueue.Num() - 1);
        OnChunkReadyForMesh.Broadcast(C);
    }
}

void UChunkManagerComponent::EnqueueGen(const FIntPoint& C)
{
    GenQueue.Add(C);
}

void UChunkManagerComponent::EnqueueMesh(const FIntPoint& C)
{
    MeshQueue.AddUnique(C);
}

void UChunkManagerComponent::LaunchGen(FIntPoint C)
{
    TSharedPtr<FChunkData> D = AllChunks.FindRef(C);
    if (!D.IsValid() || D->bIsGenerated) return;

    ActiveGenTasks++;
    FGeneratorParams P = Generator->CaptureParams();
    TWeakObjectPtr<UChunkManagerComponent> WeakSelf(this);

    Async(EAsyncExecution::ThreadPool, [D, C, P, WeakSelf]()
    {
        UWorldGeneratorComponent::GenerateChunkData(*D, P);

        AsyncTask(ENamedThreads::GameThread, [D, C, WeakSelf]()
        {
            if (!WeakSelf.IsValid()) return;
            WeakSelf->OnGenComplete(C, D);
        });
    });
}

void UChunkManagerComponent::OnGenComplete(FIntPoint C, TSharedPtr<FChunkData> D)
{
    ActiveGenTasks--;
    if (!D.IsValid()) return;

    D->bIsGenerated = true;

    TryMesh(C);

    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
            if (DX != 0 || DY != 0)
                TryMesh(FIntPoint(C.X + DX, C.Y + DY));
}

void UChunkManagerComponent::TryMesh(FIntPoint C)
{
    if (!AllChunks.Contains(C)) return;
    if (!AllNeighborsReady(C)) return;

    TSharedPtr<FChunkData> D = AllChunks.FindRef(C);
    if (!D.IsValid() || !D->bIsGenerated) return;

    EnqueueMesh(C);
}

bool UChunkManagerComponent::AllNeighborsReady(FIntPoint C) const
{
    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
        {
            if (DX == 0 && DY == 0) continue;
            const TSharedPtr<FChunkData>* D = AllChunks.Find(FIntPoint(C.X + DX, C.Y + DY));
            if (!D || !D->IsValid() || !(*D)->bIsGenerated)
                return false;
        }
    return true;
}

void UChunkManagerComponent::DesiredCoords(const FIntPoint& Center, int32 R, TArray<FIntPoint>& Out) const
{
    FIntPoint CC = BlockToChunk(Center.X, Center.Y);
    Out.Reserve((2 * R + 1) * (2 * R + 1));
    for (int32 DY = -R; DY <= R; DY++)
        for (int32 DX = -R; DX <= R; DX++)
            if (DX * DX + DY * DY <= R * R)
                Out.Add(FIntPoint(CC.X + DX, CC.Y + DY));
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

    int32 LX = BX - C.X * CHUNK_SIZE;
    int32 LY = BY - C.Y * CHUNK_SIZE;

    bool bOnBorderX = (LX == 0 || LX == CHUNK_SIZE - 1);
    bool bOnBorderY = (LY == 0 || LY == CHUNK_SIZE - 1);

    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
        {
            if (DX == 0 && DY == 0) continue;
            if (DX != 0 && !bOnBorderX) continue;
            if (DY != 0 && !bOnBorderY) continue;
            EnqueueMesh(FIntPoint(C.X + DX, C.Y + DY));
        }

    return true;
}
