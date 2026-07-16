// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "AethelgardGameMode.generated.h"

class AVoxelWorld;

UCLASS()
class AAethelgardGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AAethelgardGameMode();

    AVoxelWorld* GetVoxelWorld() const { return VoxelWorld; }

    virtual void BeginPlay() override;
    virtual void InitGameState() override;

    UFUNCTION(Exec, Category = "Save")
    void SaveGame();

    UFUNCTION(Exec, Category = "Save")
    void LoadGame(const FString& SlotName);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Terrain")
    TSubclassOf<AVoxelWorld> VoxelWorldClass;

    UPROPERTY()
    AVoxelWorld* VoxelWorld;

    UPROPERTY()
    FString CurrentSlotName;
};
