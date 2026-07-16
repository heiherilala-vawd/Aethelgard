// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "WorldGeneratorComponent.generated.h"

USTRUCT()
struct FGeneratorParams
{
    GENERATED_BODY()

    int32 Seed = 0;
    float BaseHeight = 50.0f;
    float HeightScale = 25.0f;
    float NoiseScale = 0.005f;
    int32 Octaves = 3;
    float WaterLevel = 35.0f;
};

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class UWorldGeneratorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "World")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float BaseHeight = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float HeightScale = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float NoiseScale = 0.005f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    int32 Octaves = 3;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float WaterLevel = 35.0f;

    void GenerateChunk(FChunkData& ChunkData);
    float GetHeight(int32 WorldX, int32 WorldY) const;

    FGeneratorParams CaptureParams() const
    {
        FGeneratorParams P;
        P.Seed = Seed;
        P.BaseHeight = BaseHeight;
        P.HeightScale = HeightScale;
        P.NoiseScale = NoiseScale;
        P.Octaves = Octaves;
        P.WaterLevel = WaterLevel;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
};
