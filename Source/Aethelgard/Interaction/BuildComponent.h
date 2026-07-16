// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "Interaction/InventoryComponent.h"
#include "BuildComponent.generated.h"

class AVoxelWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetBlockChanged, const FIntVector&, TargetBlock);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroyProgress, float, Progress);

UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class UBuildComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBuildComponent();

    UPROPERTY(EditAnywhere, Category = "Building")
    float ReachDistance = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Building")
    float DestroyTime = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Building")
    EBlockId SelectedBlock = EBlockId::Stone;

    UPROPERTY(BlueprintAssignable, Category = "Building")
    FOnTargetBlockChanged OnTargetBlockChanged;

    UPROPERTY(BlueprintAssignable, Category = "Building")
    FOnDestroyProgress OnDestroyProgress;

    void SetVoxelWorld(AVoxelWorld* InWorld) { TargetWorld = InWorld; }
    void SetInventory(UInventoryComponent* InInventory) { Inventory = InInventory; }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void StartDestroy();
    void TickDestroy(float DeltaTime);
    void CancelDestroy();
    void PlaceBlock();

    UFUNCTION(BlueprintCallable, Category = "Building")
    bool GetTargetBlock(FIntVector& OutBlock, FIntVector& OutPlacePos) const;

private:
    UPROPERTY()
    AVoxelWorld* TargetWorld = nullptr;

    UPROPERTY()
    UInventoryComponent* Inventory = nullptr;

    bool bIsDestroying = false;
    float CurrentDestroyProgress = 0.0f;
    FIntVector CurrentTargetBlock = FIntVector(-1, -1, -1);
    FIntVector CurrentPlacePos = FIntVector(-1, -1, -1);

    bool TraceBlock(FIntVector& OutBlock, FIntVector& OutPlacePos) const;
};
