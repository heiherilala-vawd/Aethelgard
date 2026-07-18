// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/BlockRegistry.h"
#include "VoxelWorld.generated.h"

class UChunkManagerComponent;
class UWorldGeneratorComponent;
class UGreedyMeshGenerator;
class UNetworkSystemComponent;
class UProceduralMeshComponent;

UCLASS(Placeable)
class AVoxelWorld : public AActor
{
    GENERATED_BODY()

public:
    AVoxelWorld();

    UPROPERTY(EditAnywhere, Category = "World")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, Category = "World")
    int32 ViewDistance = 4;

    UPROPERTY(EditAnywhere, Category = "World")
    float BlockScale = 100.0f;

    EBlockId GetBlock(int32 WX, int32 WY, int32 WZ) const;
    bool SetBlock(int32 WX, int32 WY, int32 WZ, EBlockId B);
    void SaveWorld(const FString& Slot);
    void LoadWorld(const FString& Slot);

    UChunkManagerComponent* GetChunkManager() const { return ChunkManager; }
    UWorldGeneratorComponent* GetWorldGenerator() const { return Generator; }

    virtual void BeginPlay() override;

protected:
    UPROPERTY(VisibleAnywhere)
    UWorldGeneratorComponent* Generator;

    UPROPERTY(VisibleAnywhere)
    UChunkManagerComponent* ChunkManager;

    UPROPERTY(VisibleAnywhere)
    UGreedyMeshGenerator* Mesher;

    UPROPERTY(VisibleAnywhere)
    UNetworkSystemComponent* Network;

    UPROPERTY()
    USceneComponent* SceneRoot;

private:
    void Init();
    void LoadBlockMaterials();
#if WITH_EDITOR
    void EnsureBlockMaterialsExist();
#endif
    void OnChunkReady(const FIntPoint& C);
    void OnChunkRemoved(const FIntPoint& C);
    void BuildSection(const FIntPoint& C);
    void ClearSection(const FIntPoint& C);
    void TickFollowPlayer();

    UProceduralMeshComponent* GetOrCreateChunkMesh(const FIntPoint& C);

    TMap<EBlockId, UMaterialInterface*> BlockMaterials;
    UMaterialInterface* DefaultMaterial = nullptr;

    TMap<FIntPoint, UProceduralMeshComponent*> ChunkMeshes;

    FTimerHandle FollowTimer;
};
