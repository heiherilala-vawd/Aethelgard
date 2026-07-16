// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoxelWorldSubsystem.generated.h"

class AVoxelWorld;

UCLASS()
class UVoxelWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    void SpawnVoxelWorld(UWorld& World);
};
