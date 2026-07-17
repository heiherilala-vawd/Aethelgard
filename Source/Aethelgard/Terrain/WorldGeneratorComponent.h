// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "WorldGeneratorComponent.generated.h"

UENUM()
enum class EBiomeType : uint8
{
    Plains = 0,
    Desert,
    Mountain,
    Forest,
    MAX UMETA(Hidden)
};

USTRUCT()
struct FBiomeParams
{
    GENERATED_BODY()

    float BaseHeight = 50.0f;
    float HeightScale = 25.0f;
    float NoiseScale = 0.005f;
    int32 Octaves = 3;
    EBlockId SurfaceBlock = EBlockId::Grass;
    EBlockId SubsurfaceBlock = EBlockId::Dirt;
    int32 SubsurfaceDepth = 3;
};

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

    float BiomeNoiseScale = 0.0015f;
    float LakeNoiseScale = 0.003f;
    float LakeThreshold = 0.85f;
    float MinLakeHeight = 50.0f;
    int32 LakeSearchRadius = 15;
    float BeachSlopeRadius = 6.0f;
    float BeachBlendPower = 2.0f;
    int32 LakeClayDepth = 3;
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

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BiomeNoiseScale = 0.0015f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float LakeNoiseScale = 0.003f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float LakeThreshold = 0.85f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float MinLakeHeight = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    int32 LakeSearchRadius = 15;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BeachSlopeRadius = 6.0f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BeachBlendPower = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    int32 LakeClayDepth = 3;

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
        P.BiomeNoiseScale = BiomeNoiseScale;
        P.LakeNoiseScale = LakeNoiseScale;
        P.LakeThreshold = LakeThreshold;
        P.MinLakeHeight = MinLakeHeight;
        P.LakeSearchRadius = LakeSearchRadius;
        P.BeachSlopeRadius = BeachSlopeRadius;
        P.BeachBlendPower = BeachBlendPower;
        P.LakeClayDepth = LakeClayDepth;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
    static float GetBiomeValue(int32 WX, int32 WY, int32 Seed);

private:
    static void GetBlendedBiomeParams(float BiomeValue, int32 WX, int32 WY, const FGeneratorParams& P,
        float& OutHeight, EBlockId& OutSurface, EBlockId& OutSubsurface, int32& OutSubsurfaceDepth);
};
