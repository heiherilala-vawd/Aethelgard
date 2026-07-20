// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/GenerationDefaults.h"

static const FBiomeParams BiomeParams[4] = {
    { EBlockId::Grass, EBlockId::Dirt, 3, true,  8.0f, GenDef::PlainsMinHeight,  GenDef::PlainsMaxHeight },
    { EBlockId::Sand,  EBlockId::Sand, 5, true,  5.0f, GenDef::DesertMinHeight,  GenDef::DesertMaxHeight },
    { EBlockId::Grass, EBlockId::Stone, 2, false, 0.0f, GenDef::MountainStart,   GenDef::MaxHeight },
    { EBlockId::Grass, EBlockId::Dirt, 4, false, 0.0f, GenDef::ForestMinHeight,  GenDef::ForestMaxHeight },
};

static float GetNoise2D(float X, float Y, float Scale, int32 Octaves, int32 Seed,
    float Persistence = 0.5f, float Lacunarity = 2.0f)
{
    static TMap<int32, FVector2D> OffsetCache;

    float Value = 0.0f, Amplitude = 1.0f, MaxAmp = 0.0f, Freq = 1.0f;
    for (int32 i = 0; i < Octaves; i++)
    {
        int32 Key = Seed + i * 7919;
        FVector2D* Cached = OffsetCache.Find(Key);
        FVector2D Oxy;
        if (Cached)
        {
            Oxy = *Cached;
        }
        else
        {
            FRandomStream Stream(Key);
            Oxy = FVector2D(Stream.FRand() * 10000.0f, Stream.FRand() * 10000.0f);
            OffsetCache.Add(Key, Oxy);
        }
        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(
            X * Scale * Freq + Oxy.X,
            Y * Scale * Freq + Oxy.Y));
        MaxAmp += Amplitude;
        Amplitude *= Persistence;
        Freq *= Lacunarity;
    }
    return Value / MaxAmp;
}

float UWorldGeneratorComponent::ComputeBaseHeight(int32 WX, int32 WY, const FGeneratorParams& P)
{
    float Macro = GetNoise2D((float)WX, (float)WY, P.MacroScale, P.MacroOctaves,
        P.Seed + (int32)ENoiseLayer::NMacro * 7919,
        P.MacroPersistence, P.MacroLacunarity);
    float BaseShape = FMath::Abs(GetNoise2D((float)WX, (float)WY, P.BaseShapeScale, 1,
        P.Seed + (int32)ENoiseLayer::NBaseShape * 7919,
        P.BaseShapePersistence, P.BaseShapeLacunarity));
    float Meso = GetNoise2D((float)WX, (float)WY, P.MesoScale, P.MesoOctaves,
        P.Seed + (int32)ENoiseLayer::NMeso * 7919,
        P.MesoPersistence, P.MesoLacunarity);
    float Micro = GetNoise2D((float)WX, (float)WY, P.MicroScale, P.MicroOctaves,
        P.Seed + (int32)ENoiseLayer::NMicro * 7919,
        P.MicroPersistence, P.MicroLacunarity);

    return  P.GlobalElevation
          + Macro * P.MacroAmplitude
          + BaseShape * P.BaseShapeAmplitude
          + Meso  * P.MesoAmplitude
          + Micro * P.MicroAmplitude;
}

int32 UWorldGeneratorComponent::VoronoiSelect(int32 WX, int32 WY, float Scale, int32 Seed)
{
    int32 CX = FMath::FloorToInt((float)WX * Scale);
    int32 CY = FMath::FloorToInt((float)WY * Scale);
    FRandomStream RNG(CX * 73856093 + CY * 19349663 + Seed * 83492791);
    static const EBiomeType MedianMap[3] = { EBiomeType::Plains, EBiomeType::Desert, EBiomeType::Forest };
    return (int32)MedianMap[RNG.RandRange(0, 2)];
}

FColumnResult UWorldGeneratorComponent::ComputeColumnAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
    FColumnResult Result;

    float Height = ComputeBaseHeight(WX, WY, P);
    Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);

    if (Height < P.SeaLevel)
    {
        float RawDepth = P.SeaLevel - Height;
        float EffectiveDepth;
        if (RawDepth <= P.SeaMaxDepth)
            EffectiveDepth = RawDepth;
        else
            EffectiveDepth = P.SeaMaxDepth + (RawDepth - P.SeaMaxDepth) * P.SeaDepthSlope;
        Height = P.SeaLevel - EffectiveDepth;
        Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
        Result.WaterSurface = (uint8)FMath::Clamp(P.SeaLevel, 0.0f, 255.0f);
        Result.Biome = EBiomeType::Plains;
        Result.Height = Height;
        return Result;
    }

    if (Height < P.MountainStart)
    {
        int32 BiomeIdx = VoronoiSelect(WX, WY, P.VoronoiScale,
            P.Seed + (int32)ENoiseLayer::NVoronoi * 7919);
        Result.Biome = static_cast<EBiomeType>(BiomeIdx);

        float HeightPreHill = Height;

        if (BiomeIdx == 0 || BiomeIdx == 1)
        {
            float HillNoise = FMath::Abs(GetNoise2D((float)WX, (float)WY, P.HillScale, 1,
                P.Seed + (int32)ENoiseLayer::NHills * 7919));
            Height += HillNoise * P.HillAmplitude;
        }

        float LakeNoise = GetNoise2D((float)WX, (float)WY, 0.01f, 1,
            P.Seed + (int32)ENoiseLayer::NLake * 7919);

        if (LakeNoise < -0.25f && HeightPreHill < P.LakeThreshold && BiomeIdx != 1)
        {
            float LakeSurface = HeightPreHill;
            float LakeFloor = Height - (float)P.LakeDepth - P.LakeDepthSlope * 4.0f;
            Height = FMath::Clamp(LakeFloor, 1.0f, P.MaxHeight);
            Result.WaterSurface = (uint8)FMath::Clamp(LakeSurface, 0.0f, 255.0f);
        }

        float MinH = BiomeParams[BiomeIdx].MinHeight;
        float MaxH = BiomeParams[BiomeIdx].MaxHeight;
        if (Height < MinH)
            Height = FMath::Lerp(Height, MinH, 0.5f);
        else if (Height > MaxH)
            Height = FMath::Lerp(Height, MaxH, 0.5f);
        Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);

        Result.Height = Height;
        return Result;
    }

    Result.Biome = EBiomeType::Mountain;
    float MountainFactor = (Height - P.MountainStart) / (P.MaxHeight - P.MountainStart);
    MountainFactor = FMath::Clamp(MountainFactor, 0.0f, 1.0f);
    float ShapeNoise = GetNoise2D((float)WX, (float)WY, P.MountainShapeScale, 2,
        P.Seed + (int32)ENoiseLayer::NMountainShape * 7919,
        P.MountainShapePersistence, P.MountainShapeLacunarity);
    Height += ShapeNoise * P.MountainShapeAmplitude * MountainFactor * MountainFactor;
    Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
    Result.Height = Height;
    return Result;
}

void UWorldGeneratorComponent::GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P)
{
    const FIntPoint& CP = ChunkData.Position;
    const int32 SX = CP.X * CHUNK_SIZE;
    const int32 SY = CP.Y * CHUNK_SIZE;

    FColumnResult Results[CHUNK_SIZE][CHUNK_SIZE];

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
            Results[X][Y] = ComputeColumnAt(SX + X, SY + Y, P);

    bool bIsShore[CHUNK_SIZE][CHUNK_SIZE];
    FMemory::Memzero(bIsShore, sizeof(bIsShore));

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            if (Results[X][Y].WaterSurface > 0) continue;
            uint8 MyH = (uint8)Results[X][Y].Height;
            const int32 DX[] = { -1, 1, 0, 0 };
            const int32 DY[] = { 0, 0, -1, 1 };
            for (int32 d = 0; d < 4; d++)
            {
                int32 NX = X + DX[d], NY = Y + DY[d];
                if (NX < 0 || NX >= CHUNK_SIZE || NY < 0 || NY >= CHUNK_SIZE) continue;
                if (Results[NX][NY].WaterSurface > 0 && MyH >= (uint8)Results[NX][NY].Height)
                { bIsShore[X][Y] = true; break; }
            }
        }

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            const FColumnResult& R = Results[X][Y];
            float Height = R.Height;

            const FBiomeParams& BP = BiomeParams[(int32)R.Biome];
            EBlockId SurfaceBlock = BP.SurfaceBlock;
            EBlockId SubsurfaceBlock = BP.SubsurfaceBlock;
            int32 SubsurfaceDepth = BP.SubsurfaceDepth;

            if (R.Biome == EBiomeType::Mountain && Height > GenDef::MountainStoneThreshold)
            {
                SurfaceBlock = EBlockId::Stone;
                SubsurfaceBlock = EBlockId::Stone;
                SubsurfaceDepth = 1;
            }

            float N1 = GetNoise2D((float)(SX + X), (float)(SY + Y), P.PerturbScale, 1,
                P.Seed + (int32)ENoiseLayer::NPerturb1 * 7919);
            float N2 = GetNoise2D((float)(SX + X), (float)(SY + Y), P.PerturbScale, 1,
                P.Seed + (int32)ENoiseLayer::NPerturb2 * 7919);
            int32 Perturb1 = (N1 > 0.3f) ? 1 : (N1 < -0.3f) ? -1 : 0;
            int32 Perturb2 = (N2 > 0.3f) ? 1 : (N2 < -0.3f) ? -1 : 0;

            int32 StoneBoundary = (int32)(Height - SubsurfaceDepth) + Perturb1;
            int32 SubBoundary = (int32)(Height - 1) + Perturb2;
            StoneBoundary = FMath::Clamp(StoneBoundary, 0, WORLD_HEIGHT - 1);
            SubBoundary = FMath::Clamp(SubBoundary, StoneBoundary + 1, WORLD_HEIGHT);

            uint8 EH = (uint8)Height;
            uint8 Top[TOP_LAYERS];

            for (int32 Layer = 0; Layer < TOP_LAYERS; Layer++)
            {
                int32 Z = (int32)EH - 1 - Layer;
                if (Z < 0)
                    Top[Layer] = (uint8)EBlockId::Stone;
                else if (Z < StoneBoundary)
                    Top[Layer] = (uint8)EBlockId::Stone;
                else if (Z < SubBoundary)
                    Top[Layer] = (uint8)SubsurfaceBlock;
                else
                    Top[Layer] = (uint8)SurfaceBlock;
            }

            if (R.WaterSurface > 0)
            {
                int32 FloorLayers = FMath::Min(P.WaterFloorDepth, TOP_LAYERS);
                for (int32 Layer = 0; Layer < FloorLayers; Layer++)
                    Top[Layer] = (uint8)EBlockId::Sand;
            }

            if (bIsShore[X][Y])
                Top[0] = (uint8)EBlockId::Sand;

            ChunkData.SetColumn(X, Y, EH, Top);
            ChunkData.SetWaterColumn(X, Y, R.WaterSurface);
        }

    ChunkData.bIsGenerated = true;
}

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData)
{
    GenerateChunkData(ChunkData, CaptureParams());
}

EBiomeType UWorldGeneratorComponent::GetBiomeAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
    return ComputeColumnAt(WX, WY, P).Biome;
}

float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const
{
    FGeneratorParams P = CaptureParams();
    return ComputeColumnAt(WorldX, WorldY, P).Height;
}
