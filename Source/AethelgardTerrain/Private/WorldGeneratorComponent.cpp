// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/GenerationDefaults.h"

static float ScoreBiome(float T, float H, float WorldHeight, const FGeneratorParams& P,
    float TempAff, float HumidAff, float HeightAff, float Adjust)
{
    float NH = FMath::Clamp((WorldHeight - 1.0f) / (P.MaxHeight - 1.0f), 0.0f, 1.0f);
    float Dist = P.TempWeight * FMath::Abs(T - TempAff)
               + P.HumidWeight * FMath::Abs(H - HumidAff)
               + P.HeightWeight * FMath::Abs(NH - HeightAff);
    return FMath::Exp(-Dist * Dist * P.AffinitySharpness) * Adjust;
}

static const FBiomeParams BiomeParams[5] = {
    { EBlockId::Grass, EBlockId::Dirt, 3, 0.0f, 0.0f, GenDef::PlainsHillAmplitude, GenDef::PlainsHillScale, true, false },
    { EBlockId::Sand,  EBlockId::Sand, 5, 0.0f, 0.0f, GenDef::DesertDuneAmplitude, GenDef::DesertDuneScale, false, true },
    { EBlockId::Grass, EBlockId::Stone, 2, GenDef::MountainRockThreshold, GenDef::MountainSnowThreshold, GenDef::MountainDetailAmplitude, GenDef::MountainDetailScale, false, false },
    { EBlockId::Grass, EBlockId::Dirt, 4, 0.0f, 0.0f, 0.0f, 0.0f, false, false },
    { EBlockId::Snow,  EBlockId::Dirt, 3, 0.0f, 0.0f, 0.0f, 0.0f, false, false },
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
        if (Cached) { Oxy = *Cached; }
        else { FRandomStream Stream(Key); Oxy = FVector2D(Stream.FRand() * 10000.0f, Stream.FRand() * 10000.0f); OffsetCache.Add(Key, Oxy); }
        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(X * Scale * Freq + Oxy.X, Y * Scale * Freq + Oxy.Y));
        MaxAmp += Amplitude;
        Amplitude *= Persistence;
        Freq *= Lacunarity;
    }
    return Value / MaxAmp;
}

float UWorldGeneratorComponent::ComputeBaseHeight(int32 WX, int32 WY, const FGeneratorParams& P)
{
    float Macro = GetNoise2D((float)WX, (float)WY, P.MacroScale, P.MacroOctaves,
        P.Seed + (int32)ENoiseLayer::NMacro * 7919, P.MacroPersistence, P.MacroLacunarity);
    float BaseShape = FMath::Abs(GetNoise2D((float)WX, (float)WY, P.BaseShapeScale, 1,
        P.Seed + (int32)ENoiseLayer::NBaseShape * 7919, P.BaseShapePersistence, P.BaseShapeLacunarity));
    return P.GlobalElevation + Macro * P.MacroAmplitude + BaseShape * P.BaseShapeAmplitude;
}

EBiomeType UWorldGeneratorComponent::SelectBiome(int32 WX, int32 WY, const FGeneratorParams& P, float& OutGradient)
{
    float Height = FMath::Clamp(ComputeBaseHeight(WX, WY, P), 1.0f, P.MaxHeight);
    if (Height < P.SeaLevel) { OutGradient = 0.0f; return EBiomeType::Plains; }
    if (Height >= P.MountainStart) { OutGradient = FMath::Clamp((Height - P.MountainStart) / P.BiomeBlendDistance, 0.0f, 1.0f); return EBiomeType::Mountain; }

    float Temp = FMath::Clamp((GetNoise2D((float)WX, (float)WY, P.TempScale, 1,
        P.Seed + (int32)ENoiseLayer::NTemperature * 7919) + 1.0f) * 0.5f, 0.0f, 1.0f);
    float Humid = FMath::Clamp((GetNoise2D((float)WX, (float)WY, P.HumidScale, 1,
        P.Seed + (int32)ENoiseLayer::NHumidity * 7919) + 1.0f) * 0.5f, 0.0f, 1.0f);
    Temp = FMath::Clamp(Temp + GetNoise2D((float)WX, (float)WY, P.TempPerturbScale, 1,
        P.Seed + (int32)ENoiseLayer::NTempPerturb * 7919) * P.TempPerturbAmplitude, 0.0f, 1.0f);
    Humid = FMath::Clamp(Humid + GetNoise2D((float)WX, (float)WY, P.HumidPerturbScale, 1,
        P.Seed + (int32)ENoiseLayer::NHumidPerturb * 7919) * P.HumidPerturbAmplitude, 0.0f, 1.0f);

    float EffGlacier = FMath::Lerp(P.GlacierThreshold, 1.0f, P.IceAgeFactor);
    EBiomeType Biome;
    if (Temp < EffGlacier) Biome = EBiomeType::Glacier;
    else
    {
        float FS = ScoreBiome(Temp, Humid, Height, P, P.ForestTempAffinity, P.ForestHumidAffinity, P.ForestHeightAffinity, P.ForestAdjust);
        float DS = ScoreBiome(Temp, Humid, Height, P, P.DesertTempAffinity, P.DesertHumidAffinity, P.DesertHeightAffinity, P.DesertAdjust);
        float PS = ScoreBiome(Temp, Humid, Height, P, P.PlainsTempAffinity, P.PlainsHumidAffinity, P.PlainsHeightAffinity, P.PlainsAdjust);
        if (FS >= DS && FS >= PS) Biome = EBiomeType::Forest;
        else if (DS >= PS) Biome = EBiomeType::Desert;
        else Biome = EBiomeType::Plains;
    }

    float dSea = Height - P.SeaLevel, dMtn = P.MountainStart - Height, dGlac = FMath::Abs(Temp - EffGlacier);
    OutGradient = FMath::Clamp(FMath::Min3(dSea, dMtn, dGlac) / P.BiomeBlendDistance, 0.0f, 1.0f);
    return Biome;
}

static float ComputeFollyContribution(int32 WX, int32 WY, float Scale, float Amplitude,
    float Bias, float t, int32 Seed)
{
    float Raw = GetNoise2D((float)WX, (float)WY, Scale, 1, Seed);
    float N = (Raw + 1.0f) * 0.5f;
    float M = FMath::Max(N - Bias, 0.0f);
    return M * Amplitude * t;
}

FColumnResult UWorldGeneratorComponent::ComputeColumnAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
    FColumnResult Result;
    int32 S = P.Seed;

    // Step 1: Macro
    float Height = FMath::Clamp(ComputeBaseHeight(WX, WY, P), 1.0f, P.MaxHeight);

    // Step 2: Mountain folly (3-layer composite)
    if (Height >= P.SeaLevel)
    {
        float t = FMath::Clamp((Height - P.SeaLevel) / 20.0f, 0.0f, 1.0f);
        int32 FS = S + (int32)ENoiseLayer::NMountainFolly * 7919;
        float C1 = ComputeFollyContribution(WX, WY, P.MountainFollyScale, P.MountainFollyAmplitude, P.MountainFollyBias, t, FS);
        float C2 = ComputeFollyContribution(WX, WY, P.MountainFollyScale * 0.8f, P.MountainFollyAmplitude * 0.8f, P.MountainFollyBias, t, FS + 1);
        float C3 = ComputeFollyContribution(WX, WY, P.MountainFollyScale * 1.1f, P.MountainFollyAmplitude * 1.1f, P.MountainFollyBias, t, FS + 2);
        Height += FMath::Max3(C1, C2, C3);
        Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
    }

    // Step 3: Zone detection (no early returns)
    bool bIsSea = (Height < P.SeaLevel);
    bool bIsMountain = (Height >= P.MountainStart);
    float HeightPreRelief = Height;

    if (bIsSea)
    {
        float SeaFloor = GetNoise2D((float)WX, (float)WY, P.SeaFloorScale, 1,
            S + (int32)ENoiseLayer::NSeaFloor * 7919) * P.SeaFloorAmplitude;
        float RawDepth = P.SeaLevel - Height;
        float EffectiveDepth = (RawDepth <= P.SeaMaxDepth) ? RawDepth : P.SeaMaxDepth + (RawDepth - P.SeaMaxDepth) * P.SeaDepthSlope;
        Height = P.SeaLevel - EffectiveDepth + SeaFloor;
        Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
        Result.WaterSurface = (uint8)FMath::Clamp(P.SeaLevel, 0.0f, 255.0f);
        Result.Biome = EBiomeType::Plains;
    }

    if (bIsMountain)
    {
        Result.Biome = EBiomeType::Mountain;
        float MtnGrad = FMath::Clamp((Height - P.MountainStart) / P.BiomeBlendDistance, 0.0f, 1.0f);
        Height += GetNoise2D((float)WX, (float)WY, P.MountainDetailScale, 2,
            S + (int32)ENoiseLayer::NMountainDetail * 7919) * P.MountainDetailAmplitude * MtnGrad;

        float MtnFactor = FMath::Clamp((Height - P.MountainStart) / (P.MaxHeight - P.MountainStart), 0.0f, 1.0f);
        float Lift = FMath::Sin(MtnFactor * PI * 0.5f);
        Height += GetNoise2D((float)WX, (float)WY, P.MountainLiftScale, 1,
            S + (int32)ENoiseLayer::NMountainDetail * 7919 + 31337) * P.MountainLiftAmplitude * Lift;

        float RoughNoise = GetNoise2D((float)WX, (float)WY, P.MountainRoughScale, 1,
            S + (int32)ENoiseLayer::NMountainRough * 7919);
        if (RoughNoise > P.MountainRoughThreshold)
        {
            float RoughMask = (RoughNoise - P.MountainRoughThreshold) / (1.0f - P.MountainRoughThreshold);
            float Detail1 = GetNoise2D((float)WX, (float)WY, P.MountainRoughDetailScale, 1,
                S + (int32)ENoiseLayer::NMountainRough * 7919 + 7901);
            float Detail2 = GetNoise2D((float)WX, (float)WY, P.MountainRoughDetailScale * 2.0f, 1,
                S + (int32)ENoiseLayer::NMountainRough * 7919 + 7907);
            Height += (Detail1 * 0.6f + Detail2 * 0.4f) * P.MountainRoughAmplitude * RoughMask * MtnFactor;
        }

        Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
    }

    // Step 4: Climate (for non-sea, non-mountain)
    if (!bIsSea && !bIsMountain)
    {
        float Temp = FMath::Clamp((GetNoise2D((float)WX, (float)WY, P.TempScale, 1,
            S + (int32)ENoiseLayer::NTemperature * 7919) + 1.0f) * 0.5f, 0.0f, 1.0f);
        float Humid = FMath::Clamp((GetNoise2D((float)WX, (float)WY, P.HumidScale, 1,
            S + (int32)ENoiseLayer::NHumidity * 7919) + 1.0f) * 0.5f, 0.0f, 1.0f);
        Temp = FMath::Clamp(Temp + GetNoise2D((float)WX, (float)WY, P.TempPerturbScale, 1,
            S + (int32)ENoiseLayer::NTempPerturb * 7919) * P.TempPerturbAmplitude, 0.0f, 1.0f);
        Humid = FMath::Clamp(Humid + GetNoise2D((float)WX, (float)WY, P.HumidPerturbScale, 1,
            S + (int32)ENoiseLayer::NHumidPerturb * 7919) * P.HumidPerturbAmplitude, 0.0f, 1.0f);

        float EffGlacier = FMath::Lerp(P.GlacierThreshold, 1.0f, P.IceAgeFactor);
        EBiomeType Biome;
        if (Temp < EffGlacier) Biome = EBiomeType::Glacier;
        else
        {
            float FS = ScoreBiome(Temp, Humid, Height, P, P.ForestTempAffinity, P.ForestHumidAffinity, P.ForestHeightAffinity, P.ForestAdjust);
            float DS = ScoreBiome(Temp, Humid, Height, P, P.DesertTempAffinity, P.DesertHumidAffinity, P.DesertHeightAffinity, P.DesertAdjust);
            float PS = ScoreBiome(Temp, Humid, Height, P, P.PlainsTempAffinity, P.PlainsHumidAffinity, P.PlainsHeightAffinity, P.PlainsAdjust);
            if (FS >= DS && FS >= PS) Biome = EBiomeType::Forest;
            else if (DS >= PS) Biome = EBiomeType::Desert;
            else Biome = EBiomeType::Plains;
        }
        Result.Biome = Biome;

        // Step 5: Gradient
        float dSea = Height - P.SeaLevel, dMtn = P.MountainStart - Height, dGlac = FMath::Abs(Temp - EffGlacier);
        float Gradient = FMath::Clamp(FMath::Min3(dSea, dMtn, dGlac) / P.BiomeBlendDistance, 0.0f, 1.0f);

        HeightPreRelief = Height;

        // Step 6: Relief
        if (Biome == EBiomeType::Plains)
            Height += FMath::Abs(GetNoise2D((float)WX, (float)WY, P.PlainsHillScale, 1,
                S + (int32)ENoiseLayer::NPlainsHill * 7919)) * P.PlainsHillAmplitude * Gradient;
        else if (Biome == EBiomeType::Desert)
            Height += FMath::Abs(GetNoise2D((float)WX, (float)WY, P.DesertDuneScale, 1,
                S + (int32)ENoiseLayer::NDesertDune * 7919)) * P.DesertDuneAmplitude * Gradient;
    }

    // Step 7: Meso + Micro (ALL columns)
    Height += GetNoise2D((float)WX, (float)WY, P.MesoScale, P.MesoOctaves,
        S + (int32)ENoiseLayer::NMeso * 7919, P.MesoPersistence, P.MesoLacunarity) * P.MesoAmplitude;
    Height += GetNoise2D((float)WX, (float)WY, P.MicroScale, P.MicroOctaves,
        S + (int32)ENoiseLayer::NMicro * 7919, P.MicroPersistence, P.MicroLacunarity) * P.MicroAmplitude;

    // Step 8: Lakes & Rivers (only if Height >= SeaLevel)
    if (Height >= P.SeaLevel)
    {
        float LakeScale = 1.0f / FMath::Max(P.LakeCircleDiameter, 1.0f);
        float RiverScale = 1.0f / FMath::Max(P.RiverCircleDiameter, 1.0f);
        float LakeNoise = GetNoise2D((float)WX, (float)WY, LakeScale, 1, S + (int32)ENoiseLayer::NLake * 7919);
        float RiverNoise = FMath::Abs(GetNoise2D((float)WX, (float)WY, RiverScale, 1, S + (int32)ENoiseLayer::NRiver * 7919));

        if (FMath::Abs(LakeNoise) > P.LakeNoiseThreshold)
        {
            float LakeFloor = Height - (float)P.LakeDepth;
            Height = FMath::Clamp(LakeFloor, 1.0f, P.MaxHeight);
            Result.WaterSurface = (uint8)FMath::Clamp(HeightPreRelief, 0.0f, 255.0f);
        }
        else if (RiverNoise < P.RiverNoiseThreshold && Height >= P.SeaLevel + 2.0f)
        {
            float RiverFactor = 1.0f - (RiverNoise / P.RiverNoiseThreshold);
            float RiverPreHeight = Height;
            Height -= RiverFactor * P.RiverDepth;
            Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
            Result.WaterSurface = (uint8)FMath::Clamp(RiverPreHeight - 1.0f, 0.0f, 255.0f);
        }
    }

    Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);

    Result.Height = Height;
    return Result;
}

void UWorldGeneratorComponent::GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P)
{
    const FIntPoint& CP = ChunkData.Position;
    const int32 SX = CP.X * CHUNK_SIZE, SY = CP.Y * CHUNK_SIZE, S = P.Seed;

    FColumnResult Results[CHUNK_SIZE][CHUNK_SIZE];
    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
            Results[X][Y] = ComputeColumnAt(SX + X, SY + Y, P);

    bool bIsBeach[CHUNK_SIZE][CHUNK_SIZE];
    bool bIsGlacierWater[CHUNK_SIZE][CHUNK_SIZE];
    FMemory::Memzero(bIsBeach, sizeof(bIsBeach));
    FMemory::Memzero(bIsGlacierWater, sizeof(bIsGlacierWater));

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
            if (Results[X][Y].WaterSurface > 0 && Results[X][Y].Biome == EBiomeType::Glacier)
                bIsGlacierWater[X][Y] = true;

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            if (Results[X][Y].WaterSurface > 0) continue;
            uint8 MyH = (uint8)Results[X][Y].Height;
            const int32 DX[] = { -1, 1, 0, 0, -1, -1, 1, 1 }, DY[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
            for (int32 d = 0; d < 8; d++)
            {
                int32 NX = X + DX[d], NY = Y + DY[d];
                if (NX < 0 || NX >= CHUNK_SIZE || NY < 0 || NY >= CHUNK_SIZE) continue;
                if (Results[NX][NY].WaterSurface > 0 && MyH >= (uint8)Results[NX][NY].Height)
                { bIsBeach[X][Y] = true; break; }
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

            if (R.Biome == EBiomeType::Mountain)
            {
                if (Height > BP.SnowHeight) SurfaceBlock = EBlockId::Snow;
                else if (Height > BP.RockHeight) SurfaceBlock = EBlockId::Stone;
            }

            float N1 = GetNoise2D((float)(SX + X), (float)(SY + Y), P.PerturbScale, 1,
                S + (int32)ENoiseLayer::NPerturb1 * 7919);
            float N2 = GetNoise2D((float)(SX + X), (float)(SY + Y), P.PerturbScale, 1,
                S + (int32)ENoiseLayer::NPerturb2 * 7919);
            int32 Perturb1 = (N1 > 0.3f) ? 1 : (N1 < -0.3f) ? -1 : 0;
            int32 Perturb2 = (N2 > 0.3f) ? 1 : (N2 < -0.3f) ? -1 : 0;

            int32 StoneBoundary = FMath::Clamp((int32)(Height - SubsurfaceDepth) + Perturb1, 0, WORLD_HEIGHT - 1);
            int32 SubBoundary = FMath::Clamp((int32)(Height - 1) + Perturb2, StoneBoundary + 1, WORLD_HEIGHT);

            uint8 EH = (uint8)Height;
            uint8 Top[TOP_LAYERS];
            for (int32 Layer = 0; Layer < TOP_LAYERS; Layer++)
            {
                int32 Z = (int32)EH - 1 - Layer;
                if (Z < 0) Top[Layer] = (uint8)EBlockId::Stone;
                else if (Z < StoneBoundary) Top[Layer] = (uint8)EBlockId::Stone;
                else if (Z < SubBoundary) Top[Layer] = (uint8)SubsurfaceBlock;
                else Top[Layer] = (uint8)SurfaceBlock;
            }

            if (R.WaterSurface > 0)
            {
                int32 FloorLayers = FMath::Min(P.WaterFloorDepth, TOP_LAYERS);
                for (int32 Layer = 0; Layer < FloorLayers; Layer++) Top[Layer] = (uint8)EBlockId::Sand;
                if (bIsGlacierWater[X][Y]) Top[0] = (uint8)EBlockId::Ice;
            }

            if (bIsBeach[X][Y]) Top[0] = (uint8)EBlockId::Sand;

            ChunkData.SetColumn(X, Y, EH, Top);
            ChunkData.SetWaterColumn(X, Y, R.WaterSurface);
        }

    ChunkData.bIsGenerated = true;
}

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData) { GenerateChunkData(ChunkData, CaptureParams()); }
EBiomeType UWorldGeneratorComponent::GetBiomeAt(int32 WX, int32 WY, const FGeneratorParams& P) { float G; return SelectBiome(WX, WY, P, G); }
float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const { return ComputeColumnAt(WorldX, WorldY, CaptureParams()).Height; }
