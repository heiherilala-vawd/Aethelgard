// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Terrain/BlockRegistry.h"
#include "AethelgardHUD.generated.h"

UCLASS()
class AAethelgardHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UFUNCTION(Exec)
    void ToggleDebug();

private:
    bool bShowDebug = true;

    void DrawDebugInfo();
    void DrawLine(float X, float& Y, const FString& Text, const FColor& Color, float Scale = 1.0f);
    static FString GetBiomeName(float BiomeValue);
    static FString GetBlockName(EBlockId BlockId);

    FString CachedPosition;
    FString CachedChunk;
    FString CachedBiome;
    FString CachedBlock;
    FString CachedSeed;
    FString CachedFPS;
    FString CachedChunks;
    int32 DebugFrameCounter = 0;
};
