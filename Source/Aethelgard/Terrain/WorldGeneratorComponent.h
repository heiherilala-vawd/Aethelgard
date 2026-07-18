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

enum class ENoiseLayer : int32
{
    NBase = 0,
    NBlend,
    NBiome1,
    NBiome2,
    NMineral1,
    NMineral2,
    NSediment,
    NMountainBase,
    NMountainRidge,
    NMountainSlope,
    NForest,
    NForestHill,
    NSand,
    NSandFalloff,
    NClaySediment,
    NValley,
    NSpillway,
    NLayer1,
    NLayer2,
    NTree,
    NMAX UMETA(Hidden)
};

enum class ELakeType : uint8
{
    None = 0,
    River,
    Basin,
    Sea
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

    float BiomeNoiseScale = 0.0000375f;

    float SeaLevel = 30.0f;
    float LakeLevel = 48.0f;
    float LakeletLevel = 45.0f;

    float BasinMarginSize = 5.0f;
    float LakeFlatMargin = 3.0f;
    float ClayDepth = 3.0f;

    float BeachWidth = 3.0f;
    float BeachSlope = 2.0f;

    int32 BedrockLayers = 3;
};

struct FColumnHeightInfo
{
    float Height = 0.0f;
    int32 DominantIdx = 0;
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
    float SeaLevel = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float LakeLevel = 48.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float LakeletLevel = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BiomeNoiseScale = 0.0000375f;

    UPROPERTY(EditAnywhere, Category = "Lake")
    float BasinMarginSize = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Lake")
    float LakeFlatMargin = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Lake")
    float ClayDepth = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Beach")
    float BeachWidth = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Beach")
    float BeachSlope = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Bedrock")
    int32 BedrockLayers = 3;

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
        P.BiomeNoiseScale = BiomeNoiseScale;
        P.SeaLevel = SeaLevel;
        P.LakeLevel = LakeLevel;
        P.LakeletLevel = LakeletLevel;
        P.BasinMarginSize = BasinMarginSize;
        P.LakeFlatMargin = LakeFlatMargin;
        P.ClayDepth = ClayDepth;
        P.BeachWidth = BeachWidth;
        P.BeachSlope = BeachSlope;
        P.BedrockLayers = BedrockLayers;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
    static float GetBiomeValue(int32 WX, int32 WY, int32 Seed);

private:
    static FColumnHeightInfo ComputeHeightAt(int32 WX, int32 WY, const FGeneratorParams& P);
    static ELakeType ClassifyZone(int32 WX, int32 WY, float Height, const FGeneratorParams& P);
};
