// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardGameMode.h"
#include "Game/AethelgardCharacter.h"
#include "Game/AethelgardGameState.h"
#include "Terrain/VoxelWorld.h"
#include "Terrain/SaveSystem.h"
#include "Terrain/NetworkSystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAethelgardGameMode, Log, All);

AAethelgardGameMode::AAethelgardGameMode()
{
    DefaultPawnClass = AAethelgardCharacter::StaticClass();
    GameStateClass = AAethelgardGameState::StaticClass();
    VoxelWorldClass = AVoxelWorld::StaticClass();
    CurrentSlotName = TEXT("AethelgardWorld");
}

void AAethelgardGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogAethelgardGameMode, Log, TEXT("AAethelgardGameMode::BeginPlay"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow,
            TEXT("AethelgardGameMode actif - Spawn VoxelWorld..."));
    }

    if (!VoxelWorld && VoxelWorldClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        VoxelWorld = GetWorld()->SpawnActor<AVoxelWorld>(VoxelWorldClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

        if (VoxelWorld)
        {
            UE_LOG(LogAethelgardGameMode, Log, TEXT("VoxelWorld spawned successfully"));
        }
        else
        {
            UE_LOG(LogAethelgardGameMode, Error, TEXT("FAILED spawn VoxelWorld"));
        }
    }
}

void AAethelgardGameMode::InitGameState()
{
    Super::InitGameState();
}

void AAethelgardGameMode::SaveGame()
{
    if (VoxelWorld)
    {
        VoxelWorld->SaveWorld(CurrentSlotName);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Monde sauvegardé"));
    }
}

void AAethelgardGameMode::LoadGame(const FString& SlotName)
{
    if (VoxelWorld)
    {
        CurrentSlotName = SlotName;
        VoxelWorld->LoadWorld(SlotName);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Monde chargé"));
    }
}
