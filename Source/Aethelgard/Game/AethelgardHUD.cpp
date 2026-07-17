// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardHUD.h"
#include "Game/AethelgardGameMode.h"
#include "Game/AethelgardGameState.h"
#include "Game/AethelgardCharacter.h"
#include "Terrain/VoxelWorld.h"
#include "Terrain/WorldGeneratorComponent.h"
#include "Terrain/ChunkManagerComponent.h"
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

FString AAethelgardHUD::GetBiomeName(float BiomeValue)
{
    static constexpr float Centers[4] = { 0.125f, 0.375f, 0.625f, 0.875f };
    static const TCHAR* Names[4] = { TEXT("Plaines"), TEXT("Desert"), TEXT("Montagne"), TEXT("Foret") };

    int32 Best = 0;
    float BestDist = MAX_FLT;
    for (int32 i = 0; i < 4; i++)
    {
        float d = FMath::Abs(BiomeValue - Centers[i]);
        float dWrap = FMath::Abs(d - 1.0f);
        float dMin = FMath::Min(d, dWrap);
        if (dMin < BestDist)
        {
            BestDist = dMin;
            Best = i;
        }
    }
    return FString(Names[Best]);
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
    AVoxelWorld* VW = GM ? GM->GetVoxelWorld() : nullptr;
    if (!VW)
    {
        for (TActorIterator<AVoxelWorld> It(GetWorld()); It; ++It)
        {
            VW = *It;
            break;
        }
    }
    if (!VW) return;

    float BS = VW->BlockScale;

    int32 BX = FMath::FloorToInt(Loc.X / BS);
    int32 BY = FMath::FloorToInt(Loc.Y / BS);
    int32 BZ = FMath::FloorToInt(Loc.Z / BS);

    int32 CX = (BX >= 0 ? BX : BX - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int32 CY = (BY >= 0 ? BY : BY - CHUNK_SIZE + 1) / CHUNK_SIZE;

    float X = 20.0f;
    float Y = 20.0f;

    FColor HeaderColor = FColor(0, 200, 255);
    FColor LabelColor  = FColor(255, 255, 255);
    FColor ValueColor = FColor(200, 255, 100);

    DrawLine(X, Y, TEXT("--- Aethelgard Debug ---"), HeaderColor, 1.2f);
    Y += 4.0f;

    DrawLine(X, Y, FString::Printf(TEXT("Position :  X=%d  Y=%d  Z=%d"), BX, BY, BZ), LabelColor);
    DrawLine(X, Y, FString::Printf(TEXT("Chunk :     X=%d  Y=%d"), CX, CY), LabelColor);

    UWorldGeneratorComponent* Gen = VW->GetWorldGenerator();
    if (Gen)
    {
        float BiomeVal = UWorldGeneratorComponent::GetBiomeValue(BX, BY, Gen->Seed);
        FString BiomeStr = GetBiomeName(BiomeVal);
        DrawLine(X, Y, FString::Printf(TEXT("Biome :     %s"), *BiomeStr), ValueColor);

        EBlockId UnderFeet = VW->GetChunkManager()->GetBlock(BX, BY, BZ);
        FString BlockStr = GetBlockName(UnderFeet);
        DrawLine(X, Y, FString::Printf(TEXT("Bloc :      %s"), *BlockStr), LabelColor);

        Y += 4.0f;

        DrawLine(X, Y, FString::Printf(TEXT("Seed :      %d"), Gen->Seed), LabelColor);
    }

    float Delta = GetWorld()->GetDeltaSeconds();
    float FPS = Delta > 0.0f ? 1.0f / Delta : 0.0f;
    DrawLine(X, Y, FString::Printf(TEXT("FPS :       %.0f"), FPS), FPS > 30.0f ? ValueColor : FColor::Red);

    int32 LoadedChunks = VW->GetChunkManager()->GetChunkCount();
    DrawLine(X, Y, FString::Printf(TEXT("Chunks :    %d"), LoadedChunks), LabelColor);
}
