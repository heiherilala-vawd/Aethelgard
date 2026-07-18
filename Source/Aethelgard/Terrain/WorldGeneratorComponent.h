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
    NLayer1,
    NLayer2,
    NTree,
    NMacro,
    NMeso,
    NMicro,
    NMAX UMETA(Hidden)
};

USTRUCT()
struct FBiomeParams
{
    GENERATED_BODY()

    float BaseHeight = 50.0f;
    float MacroNoiseScale = 0.0005f;
    float MacroAmplitude = 600.0f;
    int32 MacroOctaves = 2;
    float MesoNoiseScale = 0.025f;
    float MesoAmplitude = 15.0f;
    int32 MesoOctaves = 2;
    float MicroNoiseScale = 0.08f;
    float MicroAmplitude = 3.0f;
    int32 MicroOctaves = 1;
    EBlockId SurfaceBlock = EBlockId::Grass;
    EBlockId SubsurfaceBlock = EBlockId::Dirt;
    int32 SubsurfaceDepth = 3;
};

USTRUCT()
struct FGeneratorParams
{
    GENERATED_BODY()

    int32 Seed = 0;

    float MacroNoiseScale = 0.0005f;
    float MacroAmplitude = 600.0f;
    int32 MacroOctaves = 2;

    float MesoNoiseScale = 0.025f;
    float MesoAmplitude = 15.0f;
    int32 MesoOctaves = 2;

    float MicroNoiseScale = 0.08f;
    float MicroAmplitude = 3.0f;
    int32 MicroOctaves = 1;

    float BiomeNoiseScale = 0.00075f;

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

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroNoiseScale = 0.0005f;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroAmplitude = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    int32 MacroOctaves = 2;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoNoiseScale = 0.025f;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoAmplitude = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    int32 MesoOctaves = 2;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroNoiseScale = 0.08f;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroAmplitude = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    int32 MicroOctaves = 1;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BiomeNoiseScale = 0.00075f;

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
        P.MacroNoiseScale = MacroNoiseScale;
        P.MacroAmplitude = MacroAmplitude;
        P.MacroOctaves = MacroOctaves;
        P.MesoNoiseScale = MesoNoiseScale;
        P.MesoAmplitude = MesoAmplitude;
        P.MesoOctaves = MesoOctaves;
        P.MicroNoiseScale = MicroNoiseScale;
        P.MicroAmplitude = MicroAmplitude;
        P.MicroOctaves = MicroOctaves;
        P.BiomeNoiseScale = BiomeNoiseScale;
        P.BeachWidth = BeachWidth;
        P.BeachSlope = BeachSlope;
        P.BedrockLayers = BedrockLayers;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
    static float GetBiomeValue(int32 WX, int32 WY, int32 Seed);

private:
    static FColumnHeightInfo ComputeHeightAt(int32 WX, int32 WY, const FGeneratorParams& P);
};
