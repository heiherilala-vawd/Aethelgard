// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/WaterGenerator.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/GenerationDefaults.h"

static TMap<int32, FVector2D> VoronoiOffsetCache;

float UWaterGenerator::GetNoise2D(float X, float Y, float Scale, int32 Octaves, int32 Seed,
    float Persistence, float Lacunarity)
{
    float Value = 0.0f, Amplitude = 1.0f, MaxAmp = 0.0f, Freq = 1.0f;
    for (int32 i = 0; i < Octaves; i++)
    {
        int32 Key = Seed + i * 7919;
        FVector2D* Cached = VoronoiOffsetCache.Find(Key);
        FVector2D Oxy;
        if (Cached) { Oxy = *Cached; }
        else { FRandomStream Stream(Key); Oxy = FVector2D(Stream.FRand() * 10000.0f, Stream.FRand() * 10000.0f); VoronoiOffsetCache.Add(Key, Oxy); }
        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(X * Scale * Freq + Oxy.X, Y * Scale * Freq + Oxy.Y));
        MaxAmp += Amplitude;
        Amplitude *= Persistence;
        Freq *= Lacunarity;
    }
    return Value / MaxAmp;
}

FVoronoiResult UWaterGenerator::GetVoronoiNoise2D(float X, float Y, float Scale, int32 Seed)
{
    float SX = X * Scale;
    float SY = Y * Scale;

    int32 CellX = FMath::FloorToInt(SX);
    int32 CellY = FMath::FloorToInt(SY);

    float MinDist1 = MAX_FLT;
    float MinDist2 = MAX_FLT;
    int32 ClosestCellID = 0;

    for (int32 DY = -1; DY <= 1; DY++)
    {
        for (int32 DX = -1; DX <= 1; DX++)
        {
            int32 CX = CellX + DX;
            int32 CY = CellY + DY;

            int32 HashKey = (CX * 374761393 + CY * 668265263) ^ (Seed * 1274126177);
            FRandomStream Stream(HashKey);
            FVector2D Point(CX + Stream.FRand(), CY + Stream.FRand());

            float Distsq = FMath::Square(SX - Point.X) + FMath::Square(SY - Point.Y);

            if (Distsq < MinDist1)
            {
                MinDist2 = MinDist1;
                MinDist1 = Distsq;
                ClosestCellID = HashKey;
            }
            else if (Distsq < MinDist2)
            {
                MinDist2 = Distsq;
            }
        }
    }

    FVoronoiResult Result;
    Result.CellID = ClosestCellID;
    Result.F1 = FMath::Sqrt(MinDist1);
    Result.F2 = FMath::Sqrt(MinDist2);
    Result.Edge = Result.F2 - Result.F1;
    return Result;
}

bool UWaterGenerator::IsLakeCandidate(int32 CellID, float Probability, int32 Seed)
{
    int32 Hash = (CellID * 374761393) ^ (Seed * 1274126177);
    FRandomStream Stream(Hash);
    return Stream.FRand() < Probability;
}

float UWaterGenerator::ComputeLakeSpillPoint(int32 WX, int32 WY, float CellRadius,
    float Scale, int32 Seed, float& OutCenterHeight)
{
    float SampleRadius = CellRadius * 0.9f;
    int32 Steps = 16;

    float MinBoundaryH = MAX_FLT;
    for (int32 i = 0; i < Steps; i++)
    {
        float Angle = (float)i / (float)Steps * 2.0f * PI;
        float SX = WX + FMath::Cos(Angle) * SampleRadius;
        float SY = WY + FMath::Sin(Angle) * SampleRadius;

        float H = GetNoise2D(SX, SY, 0.0004f, 2, Seed + (int32)ENoiseLayer::NMacro * 7919);
        H = GenDef::GlobalElevation + H * GenDef::MacroAmplitude;

        float BaseShape = FMath::Abs(GetNoise2D(SX, SY, 0.001f, 1,
            Seed + (int32)ENoiseLayer::NBaseShape * 7919));
        H += BaseShape * GenDef::BaseShapeAmplitude;

        H += GetNoise2D(SX, SY, GenDef::MesoScale, GenDef::MesoOctaves,
            Seed + (int32)ENoiseLayer::NMeso * 7919) * GenDef::MesoAmplitude;
        H += GetNoise2D(SX, SY, GenDef::MicroScale, GenDef::MicroOctaves,
            Seed + (int32)ENoiseLayer::NMicro * 7919) * GenDef::MicroAmplitude;

        MinBoundaryH = FMath::Min(MinBoundaryH, H);
    }

    float CenterH = GetNoise2D((float)WX, (float)WY, 0.0004f, 2, Seed + (int32)ENoiseLayer::NMacro * 7919);
    CenterH = GenDef::GlobalElevation + CenterH * GenDef::MacroAmplitude;
    float CenterBase = FMath::Abs(GetNoise2D((float)WX, (float)WY, 0.001f, 1,
        Seed + (int32)ENoiseLayer::NBaseShape * 7919));
    CenterH += CenterBase * GenDef::BaseShapeAmplitude;
    CenterH += GetNoise2D((float)WX, (float)WY, GenDef::MesoScale, GenDef::MesoOctaves,
        Seed + (int32)ENoiseLayer::NMeso * 7919) * GenDef::MesoAmplitude;
    CenterH += GetNoise2D((float)WX, (float)WY, GenDef::MicroScale, GenDef::MicroOctaves,
        Seed + (int32)ENoiseLayer::NMicro * 7919) * GenDef::MicroAmplitude;

    OutCenterHeight = CenterH;
    return MinBoundaryH;
}

float UWaterGenerator::ComputeShoreDeformation(int32 WX, int32 WY, float Scale, float Amplitude, int32 Seed)
{
    return GetNoise2D((float)WX, (float)WY, Scale, 2, Seed) * Amplitude;
}

FWaterResult UWaterGenerator::ComputeLake(int32 WX, int32 WY, float TerrainHeight,
    const FVoronoiResult& Voronoi, const FGeneratorParams& P)
{
    FWaterResult Result;

    float CellRadius = 1.0f / FMath::Max(P.VoronoiScale, 0.001f);

    if (Voronoi.F1 > CellRadius * 0.6f)
        return Result;

    if (!IsLakeCandidate(Voronoi.CellID, P.LakeProbability, P.Seed + 777))
        return Result;

    float CenterHeight;
    float SpillPoint = ComputeLakeSpillPoint(WX, WY, CellRadius, P.VoronoiScale, P.Seed, CenterHeight);

    float MaxWaterLevel = FMath::Min(SpillPoint, CenterHeight + P.LakeMaxDepth);

    if (TerrainHeight >= MaxWaterLevel)
        return Result;

    float NormalizedDist = FMath::Clamp(Voronoi.F1 / (CellRadius * 0.5f), 0.0f, 1.0f);
    float DepthFactor = FMath::Pow(1.0f - NormalizedDist, P.LakeDepthFalloff);

    float ShoreNoise = ComputeShoreDeformation(WX, WY, P.ShoreDeformScale, P.ShoreDeformAmplitude, P.Seed + 333);
    float EffectiveSpill = SpillPoint + ShoreNoise;

    if (TerrainHeight >= EffectiveSpill)
        return Result;

    float WaterLevel = FMath::Min(EffectiveSpill, CenterHeight + P.LakeMaxDepth * DepthFactor);

    if (WaterLevel <= TerrainHeight)
        return Result;

    float Diameter = Voronoi.F1 * 2.0f / P.VoronoiScale;
    if (Diameter < P.LakeMinDiameter || Diameter > P.LakeMaxDiameter)
        return Result;

    Result.bIsWater = true;
    Result.WaterBottom = FMath::Clamp(TerrainHeight, 1.0f, P.MaxHeight);
    Result.WaterSurface = (uint8)FMath::Clamp(WaterLevel, 0.0f, 255.0f);
    return Result;
}

FWaterResult UWaterGenerator::ComputeWater(int32 WX, int32 WY, float TerrainHeight,
    const FGeneratorParams& P)
{
    FVoronoiResult Voronoi = GetVoronoiNoise2D((float)WX, (float)WY, P.VoronoiScale, P.Seed + 999);
    return ComputeLake(WX, WY, TerrainHeight, Voronoi, P);
}
