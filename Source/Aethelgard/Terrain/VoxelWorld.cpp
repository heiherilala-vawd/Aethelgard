// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/VoxelWorld.h"
#include "Terrain/WorldGeneratorComponent.h"
#include "Terrain/ChunkManagerComponent.h"
#include "Terrain/GreedyMeshGenerator.h"
#include "Terrain/NetworkSystemComponent.h"
#include "Terrain/SaveSystem.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Async/Async.h"

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

    UMaterialInterface* Mat = UMaterial::GetDefaultMaterial(MD_Surface);
    if (Mat) MainMesh->SetMaterial(0, Mat);
}

void AVoxelWorld::BeginPlay()
{
    Super::BeginPlay();
    Init();
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

void AVoxelWorld::OnChunkReady(const FIntPoint& C)
{
    BuildSection(C);
}

void AVoxelWorld::OnChunkRemoved(const FIntPoint& C)
{
    int32* SI = ActiveSections.Find(C);
    if (SI)
    {
        MainMesh->ClearMeshSection(*SI);
        ActiveSections.Remove(C);
    }
}

int32 AVoxelWorld::GetOrCreateSection(const FIntPoint& C)
{
    int32* Existing = ActiveSections.Find(C);
    if (Existing) return *Existing;

    int32 SI = NextSection++;
    ActiveSections.Add(C, SI);
    return SI;
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
    UVoxelMeshGenerator* Gen = Mesher;

    // Async mesh generation on thread pool
    Async(EAsyncExecution::ThreadPool, [this, C, Center, NB, Scale, Gen]()
    {
        FMeshSectionData Mesh;
        Gen->GenerateMesh(*Center, NB, Mesh, Scale);

        FVector Offset((float)C.X * CHUNK_SIZE * Scale, (float)C.Y * CHUNK_SIZE * Scale, 0);
        for (auto& V : Mesh.Vertices)
            V += Offset;

        // Dispatch back to game thread
        AsyncTask(ENamedThreads::GameThread, [this, C, Mesh = MoveTemp(Mesh)]()
        {
            if (!MainMesh || !IsValid(this)) return;

            if (ActiveSections.Contains(C))
            {
                int32 SI = ActiveSections[C];
                MainMesh->ClearMeshSection(SI);
                if (Mesh.Vertices.Num() > 0)
                    MainMesh->CreateMeshSection(SI, Mesh.Vertices, Mesh.Triangles,
                        Mesh.Normals, Mesh.UVs, Mesh.Colors, Mesh.Tangents, true);
            }
            else
            {
                int32 SI = NextSection++;
                ActiveSections.Add(C, SI);
                if (Mesh.Vertices.Num() > 0)
                    MainMesh->CreateMeshSection(SI, Mesh.Vertices, Mesh.Triangles,
                        Mesh.Normals, Mesh.UVs, Mesh.Colors, Mesh.Tangents, true);
            }
        });
    });
}

void AVoxelWorld::ClearSection(const FIntPoint& C)
{
    int32* SI = ActiveSections.Find(C);
    if (SI)
    {
        MainMesh->ClearMeshSection(*SI);
        ActiveSections.Remove(C);
    }
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
        Network->SetWorldSeed(LS);
        for (const auto& Ch : Mods)
            for (const auto& Chg : Ch.Changes)
                ChunkManager->SetBlock(
                    Ch.Coord.X * CHUNK_SIZE + Chg.LocalX,
                    Ch.Coord.Y * CHUNK_SIZE + Chg.LocalY,
                    Chg.LocalZ,
                    static_cast<EBlockId>(Chg.NewBlockId));
    }
}
