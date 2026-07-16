// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/BlockRegistry.h"
#include "Terrain/SaveSystem.h"
#include "VoxelWorld.generated.h"

class UChunkManagerComponent;
class UWorldGeneratorComponent;
class UGreedyMeshGenerator;
class USurfaceNetsMeshGenerator;
class UBiomeSystemComponent;
class UNetworkSystemComponent;

UENUM()
enum class EMeshGeneratorType : uint8
{
    Greedy      UMETA(DisplayName = "Greedy Meshing"),
    SurfaceNets UMETA(DisplayName = "Surface Nets (Smooth)")
};

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
    EMeshGeneratorType MeshType = EMeshGeneratorType::SurfaceNets;

    void OnPlayerMove(const FVector& PlayerPosition);

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    EBlockId GetBlock(int32 WorldX, int32 WorldY, int32 WorldZ) const;

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    bool SetBlock(int32 WorldX, int32 WorldY, int32 WorldZ, EBlockId Block);

    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveWorld(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Save")
    void LoadWorld(const FString& SlotName);

    UChunkManagerComponent* GetChunkManager() const { return ChunkManager; }
    UWorldGeneratorComponent* GetWorldGenerator() const { return WorldGenerator; }
    UNetworkSystemComponent* GetNetworkSystem() const { return NetworkSystem; }

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Terrain")
    UWorldGeneratorComponent* WorldGenerator;

    UPROPERTY(VisibleAnywhere, Category = "Terrain")
    UChunkManagerComponent* ChunkManager;

    UPROPERTY(VisibleAnywhere, Category = "Terrain")
    UBiomeSystemComponent* BiomeSystem;

    UPROPERTY(VisibleAnywhere, Category = "Terrain")
    UGreedyMeshGenerator* GreedyGenerator;

    UPROPERTY(VisibleAnywhere, Category = "Terrain")
    USurfaceNetsMeshGenerator* SurfaceNetsGenerator;

    UPROPERTY(VisibleAnywhere, Category = "Network")
    UNetworkSystemComponent* NetworkSystem;

    FTimerHandle UpdateTimerHandle;

private:
    bool bInitialized = false;
    void InitializeWorld();
    void OnUpdateTimer();
    void ApplyModifications(const TArray<FChunkSaveData>& Modifications);
    void OnNetworkBlockChange(int32 X, int32 Y, int32 Z, uint8 NewBlockId);
};
