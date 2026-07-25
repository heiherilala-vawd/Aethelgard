// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardHUD.h"
#include "Game/AethelgardGameMode.h"
#include "Game/AethelgardGameState.h"
#include "Game/AethelgardCharacter.h"
#include "AethelgardTerrain/VoxelWorld.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/ChunkManagerComponent.h"
#include "AethelgardTerrain/CoordinateUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void AAethelgardHUD::ToggleDebug()
{
    bShowDebug = !bShowDebug;
}

void AAethelgardHUD::DrawHUD()
{
    Super::DrawHUD();
    if (bShowDebug)
        DrawDebugInfo();
}

void AAethelgardHUD::DrawLine(float X, float& Y, const FString& Text, const FColor& Color, float Scale)
{
    Canvas->SetDrawColor(Color);
    Canvas->DrawText(GEngine->GetSmallFont(), Text, X, Y, Scale);
    Y += 18.0f * Scale;
}

FString AAethelgardHUD::GetBiomeName(EBiomeType Biome)
{
    switch (Biome)
    {
    case EBiomeType::Plains:           return TEXT("Plaines");
    case EBiomeType::Desert:           return TEXT("Desert");
    case EBiomeType::Forest:           return TEXT("Foret");
    case EBiomeType::ColdPlace:        return TEXT("Endroit Froid");
    case EBiomeType::IceMountain:      return TEXT("Mont. Glacee");
    case EBiomeType::HumidMountain:    return TEXT("Mont. Humide");
    case EBiomeType::ClassicMountain:  return TEXT("Montagne");
    default:                           return TEXT("Mer");
    }
}

FString AAethelgardHUD::GetBlockName(EBlockId BlockId)
{
    return GetBlockDef(BlockId).Name.ToString();
}

void AAethelgardHUD::DrawDebugInfo()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();

    AAethelgardGameMode* GM = Cast<AAethelgardGameMode>(GetWorld()->GetAuthGameMode());
    AVoxelWorld* VW = CachedVoxelWorld.Get();
    if (!VW)
    {
        VW = GM ? GM->GetVoxelWorld() : nullptr;
        if (!VW)
        {
            for (TActorIterator<AVoxelWorld> It(GetWorld()); It; ++It)
            {
                VW = *It;
                break;
            }
        }
        if (VW)
            CachedVoxelWorld = VW;
    }
    if (!VW) return;

    float BS = VW->BlockScale;
    FIntPoint BP = TerrainCoords::WorldToBlock(Loc, BS);
    int32 BX = BP.X, BY = BP.Y;
    int32 BZ = FMath::FloorToInt(Loc.Z / BS);

    FIntPoint ChunkP = TerrainCoords::BlockToChunk(BX, BY);
    int32 CX = ChunkP.X, CY = ChunkP.Y;

    float Delta = GetWorld()->GetDeltaSeconds();
    float FPS = Delta > 0.0f ? 1.0f / Delta : 0.0f;

    DebugFrameCounter++;
    if (DebugFrameCounter % 15 == 0)
    {
        CachedPosition = FString::Printf(TEXT("Position :  X=%d  Y=%d  Z=%d"), BX, BY, BZ);
        CachedChunk = FString::Printf(TEXT("Chunk :     X=%d  Y=%d"), CX, CY);
        CachedFPS = FString::Printf(TEXT("FPS :       %.0f"), FPS);
        CachedChunks = FString::Printf(TEXT("Chunks :    %d"), VW->GetChunkManager()->GetChunkCount());

        UWorldGeneratorComponent* Gen = VW->GetWorldGenerator();
        if (Gen)
        {
            FColumnResult CR = UWorldGeneratorComponent::ComputeRawColumnAt(BX, BY, Gen->CaptureParams());

            CachedBiome = FString::Printf(TEXT("Biome :     %s"), *GetBiomeName(CR.Biome));

            EBlockId UnderFeet = VW->GetChunkManager()->GetBlock(BX, BY, BZ);
            CachedBlock = FString::Printf(TEXT("Bloc :      %s"), *GetBlockName(UnderFeet));

            CachedSeed = FString::Printf(TEXT("Seed :      %d"), Gen->Seed);

            CachedTemperature = FString::Printf(TEXT("Temp :      %.2f"), CR.Temperature);
            CachedHumidity = FString::Printf(TEXT("Humid :     %.2f"), CR.Humidity);
        }
    }

    float X = 20.0f;
    float Y = 20.0f;

    FColor HeaderColor = FColor(0, 200, 255);
    FColor LabelColor  = FColor(255, 255, 255);
    FColor ValueColor = FColor(200, 255, 100);

    DrawLine(X, Y, TEXT("--- Aethelgard Debug ---"), HeaderColor, 1.2f);
    Y += 4.0f;

    DrawLine(X, Y, CachedPosition, LabelColor);
    DrawLine(X, Y, CachedChunk, LabelColor);

    UWorldGeneratorComponent* Gen = VW->GetWorldGenerator();
    if (Gen)
    {
        DrawLine(X, Y, CachedBiome, ValueColor);
        DrawLine(X, Y, CachedTemperature, LabelColor);
        DrawLine(X, Y, CachedHumidity, LabelColor);
        DrawLine(X, Y, CachedBlock, LabelColor);
        Y += 4.0f;
        DrawLine(X, Y, CachedSeed, LabelColor);
    }

    DrawLine(X, Y, CachedFPS, FPS > 30.0f ? ValueColor : FColor::Red);
    DrawLine(X, Y, CachedChunks, LabelColor);
}
