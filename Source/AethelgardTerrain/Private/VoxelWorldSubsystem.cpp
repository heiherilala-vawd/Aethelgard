// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/VoxelWorldSubsystem.h"
#include "AethelgardTerrain/VoxelWorld.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogVoxelWorldSS, Log, All);

void UVoxelWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.WorldType != EWorldType::Game && InWorld.WorldType != EWorldType::PIE)
        return;

    UE_LOG(LogVoxelWorldSS, Log, TEXT("VoxelWorldSubsystem::OnWorldBeginPlay"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan,
            TEXT("VoxelWorldSubsystem actif"));
    }

    SpawnVoxelWorld(InWorld);
}

void UVoxelWorldSubsystem::SpawnVoxelWorld(UWorld& World)
{
    for (TActorIterator<AVoxelWorld> It(&World); It; ++It)
    {
        UE_LOG(LogVoxelWorldSS, Log, TEXT("VoxelWorld déjà présent dans le monde, pas de création automatique"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AVoxelWorld* VoxelWorld = World.SpawnActor<AVoxelWorld>(
        AVoxelWorld::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (VoxelWorld)
    {
        UE_LOG(LogVoxelWorldSS, Log, TEXT("VoxelWorld auto-spawné avec succès"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green,
                TEXT("VoxelWorld auto-spawné"));
        }
    }
    else
    {
        UE_LOG(LogVoxelWorldSS, Error, TEXT("ÉCHEC auto-spawn VoxelWorld"));
    }
}
