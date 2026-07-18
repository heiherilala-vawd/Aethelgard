// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardGameMode.h"
#include "Game/AethelgardCharacter.h"
#include "Game/AethelgardGameState.h"
#include "Game/AethelgardHUD.h"
#include "Terrain/VoxelWorld.h"
#include "Terrain/SaveSystem.h"
#include "Interaction/InventoryComponent.h"
#include "Terrain/NetworkSystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAethelgardGameMode, Log, All);

AAethelgardGameMode::AAethelgardGameMode()
{
    DefaultPawnClass = AAethelgardCharacter::StaticClass();
    GameStateClass = AAethelgardGameState::StaticClass();
    HUDClass = AAethelgardHUD::StaticClass();
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
    if (!VoxelWorld) return;

    TArray<FInventorySlotSaveData> InventoryData;
    APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (Pawn)
    {
        UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
        if (Inv) InventoryData = Inv->GetSaveData();
    }

    VoxelWorld->SaveWorld(CurrentSlotName, InventoryData);
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Monde sauvegardé"));
}

void AAethelgardGameMode::LoadGame(const FString& SlotName)
{
    if (!VoxelWorld) return;

    CurrentSlotName = SlotName;

    TArray<FInventorySlotSaveData> InventoryData;
    VoxelWorld->LoadWorld(SlotName, InventoryData);

    APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (Pawn)
    {
        UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
        if (Inv) Inv->LoadFromSaveData(InventoryData);
    }

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Monde chargé"));
}
