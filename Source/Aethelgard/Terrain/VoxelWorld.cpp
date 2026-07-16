// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/VoxelWorld.h"
#include "Terrain/WorldGeneratorComponent.h"
#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/GreedyMeshGenerator.h"
#include "Terrain/SurfaceNetsMeshGenerator.h"
#include "Terrain/BiomeSystemComponent.h"
#include "Terrain/NetworkSystemComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogVoxelWorld, Log, All);

AVoxelWorld::AVoxelWorld()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    WorldGenerator = CreateDefaultSubobject<UWorldGeneratorComponent>(TEXT("WorldGenerator"));
    ChunkManager = CreateDefaultSubobject<UChunkManagerComponent>(TEXT("ChunkManager"));
    BiomeSystem = CreateDefaultSubobject<UBiomeSystemComponent>(TEXT("BiomeSystem"));
    GreedyGenerator = CreateDefaultSubobject<UGreedyMeshGenerator>(TEXT("GreedyGenerator"));
    SurfaceNetsGenerator = CreateDefaultSubobject<USurfaceNetsMeshGenerator>(TEXT("SurfaceNetsGenerator"));
    NetworkSystem = CreateDefaultSubobject<UNetworkSystemComponent>(TEXT("NetworkSystem"));
}

void AVoxelWorld::InitializeWorld()
{
    if (bInitialized)
        return;
    bInitialized = true;

    UE_LOG(LogVoxelWorld, Log, TEXT("=== VoxelWorld INIT - Seed: %d ==="), Seed);

    WorldGenerator->Seed = Seed;
    WorldGenerator->SetBiomeSystem(BiomeSystem);
    BiomeSystem->SetSeed(Seed);
    NetworkSystem->SetWorldSeed(Seed);

    if (MeshType == EMeshGeneratorType::SurfaceNets)
        ChunkManager->SetMeshGenerator(SurfaceNetsGenerator);
    else
        ChunkManager->SetMeshGenerator(GreedyGenerator);

    ChunkManager->SetWorldGenerator(WorldGenerator);
    ChunkManager->ViewDistance = ViewDistance;

    NetworkSystem->OnBlockChangeReceived.AddUObject(this, &AVoxelWorld::OnNetworkBlockChange);

    ChunkManager->UpdatePlayerPosition(FVector::ZeroVector);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green,
            FString::Printf(TEXT("VoxelWorld initialisé - Seed: %d"), Seed));
    }
}

void AVoxelWorld::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    InitializeWorld();
}

void AVoxelWorld::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogVoxelWorld, Log, TEXT("VoxelWorld::BeginPlay"));

    InitializeWorld();

    GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &AVoxelWorld::OnUpdateTimer, 0.25f, true);
}

void AVoxelWorld::OnPlayerMove(const FVector& PlayerPosition)
{
    ChunkManager->UpdatePlayerPosition(PlayerPosition);
}

EBlockId AVoxelWorld::GetBlock(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
    return ChunkManager->GetBlock(WorldX, WorldY, WorldZ);
}

bool AVoxelWorld::SetBlock(int32 WorldX, int32 WorldY, int32 WorldZ, EBlockId Block)
{
    EBlockId OldBlock = GetBlock(WorldX, WorldY, WorldZ);
    bool bResult = ChunkManager->SetBlock(WorldX, WorldY, WorldZ, Block);

    if (bResult)
    {
        USaveSystem* SaveSystem = GetGameInstance()->GetSubsystem<USaveSystem>();
        if (SaveSystem)
        {
            SaveSystem->RecordModification(FIntVector(WorldX, WorldY, WorldZ), OldBlock, Block);
        }
    }

    return bResult;
}

void AVoxelWorld::SaveWorld(const FString& SlotName)
{
    USaveSystem* SaveSystem = GetGameInstance()->GetSubsystem<USaveSystem>();
    if (SaveSystem)
    {
        SaveSystem->SaveWorld(SlotName, Seed);
    }
}

void AVoxelWorld::LoadWorld(const FString& SlotName)
{
    USaveSystem* SaveSystem = GetGameInstance()->GetSubsystem<USaveSystem>();
    if (!SaveSystem)
        return;

    int32 LoadedSeed = 0;
    TArray<FChunkSaveData> Modifications;

    if (SaveSystem->LoadWorld(SlotName, LoadedSeed, Modifications))
    {
        Seed = LoadedSeed;
        InitializeWorld();
        ChunkManager->UpdatePlayerPosition(FVector::ZeroVector);
        ApplyModifications(Modifications);
    }
}

void AVoxelWorld::ApplyModifications(const TArray<FChunkSaveData>& Modifications)
{
    for (const FChunkSaveData& ChunkData : Modifications)
    {
        for (const FBlockChange& Change : ChunkData.Changes)
        {
            int32 WX = ChunkData.Coord.X * CHUNK_SIZE + Change.LocalPos.X;
            int32 WY = ChunkData.Coord.Y * CHUNK_SIZE + Change.LocalPos.Y;
            int32 WZ = ChunkData.Coord.Z * CHUNK_SIZE + Change.LocalPos.Z;

            ChunkManager->SetBlock(WX, WY, WZ, static_cast<EBlockId>(Change.NewBlockId));
        }
    }
}

void AVoxelWorld::OnNetworkBlockChange(int32 X, int32 Y, int32 Z, uint8 NewBlockId)
{
    EBlockId OldBlock = GetBlock(X, Y, Z);
    EBlockId NewBlock = static_cast<EBlockId>(NewBlockId);

    ChunkManager->SetBlock(X, Y, Z, NewBlock);

    USaveSystem* SaveSystem = GetGameInstance()->GetSubsystem<USaveSystem>();
    if (SaveSystem)
    {
        SaveSystem->RecordModification(FIntVector(X, Y, Z), OldBlock, NewBlock);
    }
}

void AVoxelWorld::OnUpdateTimer()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        OnPlayerMove(PC->GetPawn()->GetActorLocation());
    }
}
