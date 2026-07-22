// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGeneratorParams;

struct AETHELGARDTERRAIN_API FVoronoiResult
{
    int32 CellID = 0;
    float F1 = 0.0f;
    float F2 = 0.0f;
    float Edge = 0.0f;
};

struct AETHELGARDTERRAIN_API FWaterResult
{
    bool bIsWater = false;
    float WaterBottom = 0.0f;
    uint8 WaterSurface = 0;
};

class AETHELGARDTERRAIN_API UWaterGenerator
{
public:
    static FVoronoiResult GetVoronoiNoise2D(float X, float Y, float Scale, int32 Seed);
    static FWaterResult ComputeWater(int32 WX, int32 WY, float TerrainHeight, const FGeneratorParams& P);

private:
    static float GetNoise2D(float X, float Y, float Scale, int32 Octaves, int32 Seed,
        float Persistence = 0.5f, float Lacunarity = 2.0f);
    static FWaterResult ComputeLake(int32 WX, int32 WY, float TerrainHeight,
        const FVoronoiResult& Voronoi, const FGeneratorParams& P);
    static bool IsLakeCandidate(int32 CellID, float Probability, int32 Seed);
    static float ComputeLakeSpillPoint(int32 WX, int32 WY, float CellRadius,
        float Scale, int32 Seed, float& OutCenterHeight);
    static float ComputeShoreDeformation(int32 WX, int32 WY, float Scale, float Amplitude, int32 Seed);
};
