// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/VoxelWorld.h"
#include "Terrain/WorldGeneratorComponent.h"
#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/GreedyMeshGenerator.h"
#include "Terrain/NetworkSystemComponent.h"
#include "Terrain/SaveSystem.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Async/Async.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

AVoxelWorld::AVoxelWorld()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    Generator = CreateDefaultSubobject<UWorldGeneratorComponent>(TEXT("Generator"));
    ChunkManager = CreateDefaultSubobject<UChunkManagerComponent>(TEXT("ChunkManager"));
    Mesher = CreateDefaultSubobject<UGreedyMeshGenerator>(TEXT("Mesher"));
    Network = CreateDefaultSubobject<UNetworkSystemComponent>(TEXT("Network"));

    MainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MainMesh"));
    SetRootComponent(MainMesh);
    MainMesh->SetCastShadow(false);
    MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MainMesh->bUseComplexAsSimpleCollision = false;

    UMaterialInterface* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface);
    if (DefaultMat) MainMesh->SetMaterial(0, DefaultMat);
}

void AVoxelWorld::BeginPlay()
{
    Super::BeginPlay();
    Init();
    LoadBlockMaterials();
    GetWorldTimerManager().SetTimer(FollowTimer, this, &AVoxelWorld::TickFollowPlayer, 0.3f, true);
}

void AVoxelWorld::Init()
{
    Generator->Seed = Seed;
    Network->SetWorldSeed(Seed);
    ChunkManager->SetWorldGenerator(Generator);
    ChunkManager->ViewDistance = ViewDistance;
    ChunkManager->OnChunkReadyForMesh.AddUObject(this, &AVoxelWorld::OnChunkReady);
    ChunkManager->OnChunkRemoved.AddUObject(this, &AVoxelWorld::OnChunkRemoved);
}

void AVoxelWorld::LoadBlockMaterials()
{
    DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);

#if WITH_EDITOR
    EnsureBlockMaterialsExist();
#endif

    for (int32 i = 1; i < static_cast<int32>(EBlockId::MAX); i++)
    {
        EBlockId Id = static_cast<EBlockId>(i);
        const FBlockDefinition& Def = GetBlockDef(Id);

        if (Def.MaterialPath.IsNone()) continue;

        FString Path = Def.MaterialPath.ToString();
        UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (Mat)
            BlockMaterials.Add(Id, Mat);
    }
}

#if WITH_EDITOR
void AVoxelWorld::EnsureBlockMaterialsExist()
{
    static const FString DefaultMatPath = TEXT("/Game/Materials/M_Default");

    UMaterialInterface* DefaultMat = LoadObject<UMaterialInterface>(nullptr, *DefaultMatPath, nullptr, LOAD_NoWarn);
    if (!DefaultMat)
    {
        UE_LOG(LogTemp, Warning, TEXT("M_Default not found, cannot auto-create block materials"));
        return;
    }

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    for (int32 i = 1; i < static_cast<int32>(EBlockId::MAX); i++)
    {
        EBlockId Id = static_cast<EBlockId>(i);
        const FBlockDefinition& Def = GetBlockDef(Id);

        if (Def.MaterialPath.IsNone()) continue;

        FString DestPath = Def.MaterialPath.ToString();

        if (LoadObject<UMaterialInterface>(nullptr, *DestPath, nullptr, LOAD_NoWarn)) continue;

        FString AssetName = FPaths::GetBaseFilename(DestPath);
        FString PackagePath = FPaths::GetPath(DestPath);
        AssetTools.DuplicateAsset(AssetName, PackagePath, DefaultMat);

        UE_LOG(LogTemp, Log, TEXT("Created material: %s (copy of M_Default)"), *DestPath);
    }
}
#endif

void AVoxelWorld::OnChunkReady(const FIntPoint& C)
{
    BuildSection(C);
}

void AVoxelWorld::OnChunkRemoved(const FIntPoint& C)
{
    ClearSection(C);
}

void AVoxelWorld::BuildSection(const FIntPoint& C)
{
    TSharedPtr<FChunkData> Center = ChunkManager->GetChunk(C);
    if (!Center.IsValid() || !Center->bIsGenerated) return;

    TMap<FIntPoint, TSharedPtr<FChunkData>> NB;
    for (int32 DY = -1; DY <= 1; DY++)
        for (int32 DX = -1; DX <= 1; DX++)
        {
            if (DX == 0 && DY == 0) continue;
            FIntPoint N(C.X + DX, C.Y + DY);
            auto D = ChunkManager->GetChunk(N);
            if (D.IsValid() && D->bIsGenerated) NB.Add(N, D);
        }

    float Scale = BlockScale;
    TWeakObjectPtr<UGreedyMeshGenerator> WeakGen(Mesher);

    Async(EAsyncExecution::ThreadPool, [this, C, Center, NB, Scale, WeakGen]()
    {
        if (!WeakGen.IsValid()) return;
        TMap<EBlockId, FMeshSectionData> Sections;
        WeakGen->GenerateMesh(*Center, NB, Sections, Scale);

        FVector Offset((float)C.X * CHUNK_SIZE * Scale, (float)C.Y * CHUNK_SIZE * Scale, 0);
        for (auto& Pair : Sections)
            for (auto& V : Pair.Value.Vertices)
                V += Offset;

        AsyncTask(ENamedThreads::GameThread, [this, C, Sections = MoveTemp(Sections)]()
        {
            if (!MainMesh || !IsValid(this)) return;

            ClearSection(C);

            TMap<EBlockId, int32>& ChunkSections = ActiveSections.Add(C);

            for (auto& Pair : Sections)
            {
                EBlockId BlockType = Pair.Key;
                const FMeshSectionData& Mesh = Pair.Value;

                if (Mesh.Vertices.Num() == 0) continue;

                int32 SI = NextSection++;
                ChunkSections.Add(BlockType, SI);

                MainMesh->CreateMeshSection(SI, Mesh.Vertices, Mesh.Triangles,
                    Mesh.Normals, Mesh.UVs, Mesh.Colors, Mesh.Tangents, true);

                UMaterialInterface** MatPtr = BlockMaterials.Find(BlockType);
                UMaterialInterface* Mat = MatPtr ? *MatPtr : DefaultMaterial;
                if (Mat)
                    MainMesh->SetMaterial(SI, Mat);
            }
        });
    });
}

void AVoxelWorld::ClearSection(const FIntPoint& C)
{
    TMap<EBlockId, int32>* ChunkSections = ActiveSections.Find(C);
    if (!ChunkSections) return;

    for (auto& Pair : *ChunkSections)
        MainMesh->ClearMeshSection(Pair.Value);

    ActiveSections.Remove(C);
}

void AVoxelWorld::TickFollowPlayer()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        FVector Pos = PC->GetPawn()->GetActorLocation();
        FIntPoint BC((int32)(Pos.X / BlockScale), (int32)(Pos.Y / BlockScale));
        ChunkManager->UpdateCenter(BC);
    }
}

EBlockId AVoxelWorld::GetBlock(int32 WX, int32 WY, int32 WZ) const
{
    return ChunkManager->GetBlock((int32)(WX / BlockScale), (int32)(WY / BlockScale), (int32)(WZ / BlockScale));
}

bool AVoxelWorld::SetBlock(int32 WX, int32 WY, int32 WZ, EBlockId B)
{
    return ChunkManager->SetBlock((int32)(WX / BlockScale), (int32)(WY / BlockScale), (int32)(WZ / BlockScale), B);
}

void AVoxelWorld::SaveWorld(const FString& Slot)
{
    USaveSystem* SS = GetGameInstance()->GetSubsystem<USaveSystem>();
    if (SS) SS->SaveWorld(Slot, Seed);
}

void AVoxelWorld::LoadWorld(const FString& Slot)
{
    USaveSystem* SS = GetGameInstance()->GetSubsystem<USaveSystem>();
    if (!SS) return;
    int32 LS = 0;
    TArray<FChunkSaveData> Mods;
    if (SS->LoadWorld(Slot, LS, Mods))
    {
        Seed = LS;
        Generator->Seed = LS;
        Network->SetWorldSeed(Seed);
        for (const auto& Ch : Mods)
            for (const auto& Chg : Ch.Changes)
                ChunkManager->SetBlock(
                    Ch.Coord.X * CHUNK_SIZE + Chg.LocalX,
                    Ch.Coord.Y * CHUNK_SIZE + Chg.LocalY,
                    Chg.LocalZ,
                    static_cast<EBlockId>(Chg.NewBlockId));
    }
}
