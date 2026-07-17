// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/WorldGeneratorComponent.h"

static float GetNoise2D(float X, float Y, float Scale, int32 Octaves, int32 Seed)
{
    float Value = 0.0f, Amplitude = 1.0f, MaxAmp = 0.0f, Freq = 1.0f;
    for (int32 i = 0; i < Octaves; i++)
    {
        FRandomStream Stream(Seed + i * 7919);
        float OX = Stream.FRand() * 10000.0f, OY = Stream.FRand() * 10000.0f;
        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(
            X * Scale * Freq + OX,
            Y * Scale * Freq + OY));
        MaxAmp += Amplitude;
        Amplitude *= 0.5f;
        Freq *= 2.0f;
    }
    return Value / MaxAmp;
}

static float ComputeHeightAt(int32 WX, int32 WY, float BaseHeight, float HeightScale,
    float NoiseScale, int32 Octaves, int32 Seed)
{
    float Noise = GetNoise2D((float)WX, (float)WY, NoiseScale, Octaves, Seed);
    return BaseHeight + Noise * HeightScale;
}

static constexpr float BiomeCenters[4] = { 0.125f, 0.375f, 0.625f, 0.875f };

static const FBiomeParams BiomeParams[4] = {
    { 44.0f, 10.0f, 0.010f, 3, EBlockId::Grass,   EBlockId::Dirt,  3 },  // Plains
    { 48.0f, 12.0f, 0.007f, 2, EBlockId::Sand,    EBlockId::Sand,  5 },  // Desert
    { 60.0f, 35.0f, 0.005f, 4, EBlockId::Grass,   EBlockId::Stone, 2 },  // Mountain
    { 50.0f, 18.0f, 0.006f, 3, EBlockId::Grass,   EBlockId::Dirt,  4 },  // Forest
};

float UWorldGeneratorComponent::GetBiomeValue(int32 WX, int32 WY, int32 Seed)
{
    float Noise = GetNoise2D((float)WX, (float)WY, 0.0015f, 2, Seed + 50000);
    return FMath::Fmod(FMath::Fmod(Noise, 1.0f) + 1.0f, 1.0f);
}

void UWorldGeneratorComponent::GetBlendedBiomeParams(float BiomeValue, int32 WX, int32 WY,
    const FGeneratorParams& P, float& OutHeight, EBlockId& OutSurface, EBlockId& OutSubsurface, int32& OutSubsurfaceDepth)
{
    float MinDist1 = MAX_FLT, MinDist2 = MAX_FLT;
    int32 Idx1 = 0, Idx2 = 1;

    for (int32 i = 0; i < 4; i++)
    {
        float d = FMath::Abs(BiomeValue - BiomeCenters[i]);
        float dWrap = FMath::Abs(d - 1.0f);
        float dMin = FMath::Min(d, dWrap);

        if (dMin < MinDist1)
        {
            MinDist2 = MinDist1;
            Idx2 = Idx1;
            MinDist1 = dMin;
            Idx1 = i;
        }
        else if (dMin < MinDist2)
        {
            MinDist2 = dMin;
            Idx2 = i;
        }
    }

    float TotalDist = MinDist1 + MinDist2;
    float BlendWeight = (TotalDist > 0.001f) ? MinDist1 / TotalDist : 0.5f;

    const FBiomeParams& B1 = BiomeParams[Idx1];
    const FBiomeParams& B2 = BiomeParams[Idx2];

    float H1 = ComputeHeightAt(WX, WY, B1.BaseHeight, B1.HeightScale, B1.NoiseScale, B1.Octaves, P.Seed);
    float H2 = ComputeHeightAt(WX, WY, B2.BaseHeight, B2.HeightScale, B2.NoiseScale, B2.Octaves, P.Seed);

    OutHeight = FMath::Lerp(H1, H2, BlendWeight);

    int32 DominantIdx = (BlendWeight < 0.5f) ? Idx1 : Idx2;
    const FBiomeParams& Dominant = BiomeParams[DominantIdx];

    OutSurface = Dominant.SurfaceBlock;
    OutSubsurface = Dominant.SubsurfaceBlock;
    OutSubsurfaceDepth = Dominant.SubsurfaceDepth;

    if (DominantIdx == 2 && OutHeight > 75.0f)
    {
        OutSurface = EBlockId::Stone;
        OutSubsurface = EBlockId::Stone;
        OutSubsurfaceDepth = 1;
    }
}

float UWorldGeneratorComponent::ComputeLakeDepth(int32 WX, int32 WY, float Height, const FGeneratorParams& P)
{
    if (Height <= P.MinLakeHeight)
        return -1.0f;

    float LakeNoise = GetNoise2D((float)WX, (float)WY, P.LakeNoiseScale, 2, P.Seed + 90000);
    float LakeNorm = FMath::Fmod(FMath::Fmod(LakeNoise, 1.0f) + 1.0f, 1.0f);

    if (LakeNorm <= P.LakeThreshold)
        return -1.0f;

    float DepthNoise = GetNoise2D((float)WX, (float)WY, 0.01f, 1, P.Seed + 120000);
    float DepthNorm = FMath::Fmod(FMath::Fmod(DepthNoise, 1.0f) + 1.0f, 1.0f);
    float Depth = 3.0f + DepthNorm * 3.0f;

    float LakeSurface = FMath::Max(Height - Depth, P.WaterLevel + 2.0f);
    float ActualDepth = Height - LakeSurface;

    if (ActualDepth < 2.0f)
        return -1.0f;

    return ActualDepth;
}

void UWorldGeneratorComponent::GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P)
{
    const FIntPoint& CP = ChunkData.Position;
    const int32 SX = CP.X * CHUNK_SIZE;
    const int32 SY = CP.Y * CHUNK_SIZE;

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
    {
        int32 WY = SY + Y;
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            int32 WX = SX + X;

            float BiomeValue = GetBiomeValue(WX, WY, P.Seed);
            float Height;
            EBlockId SurfaceBlock, SubsurfaceBlock;
            int32 SubsurfaceDepth;

            GetBlendedBiomeParams(BiomeValue, WX, WY, P,
                Height, SurfaceBlock, SubsurfaceBlock, SubsurfaceDepth);

            float LakeDepth = ComputeLakeDepth(WX, WY, Height, P);
            float EffectiveHeight = Height;

            if (LakeDepth > 0.0f)
            {
                float LakeSurface = Height - LakeDepth;
                float LakeFloor = LakeSurface - 2.0f;
                EffectiveHeight = LakeFloor;
            }

            for (int32 Z = 0; Z < WORLD_HEIGHT; Z++)
            {
                EBlockId Block;

                if ((float)Z < EffectiveHeight - (float)SubsurfaceDepth)
                {
                    Block = EBlockId::Stone;
                }
                else if ((float)Z < EffectiveHeight - 1.0f)
                {
                    Block = SubsurfaceBlock;
                }
                else if ((float)Z < EffectiveHeight)
                {
                    Block = SurfaceBlock;
                }
                else if (LakeDepth > 0.0f && (float)Z <= Height - LakeDepth)
                {
                    Block = EBlockId::Water;
                }
                else if ((float)Z <= P.WaterLevel)
                {
                    Block = EBlockId::Water;
                }
                else
                {
                    Block = EBlockId::Air;
                }

                ChunkData.SetBlock(X, Y, Z, Block);
            }
        }
    }

    ChunkData.bIsGenerated = true;
}

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData)
{
    GenerateChunkData(ChunkData, CaptureParams());
}

float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const
{
    FGeneratorParams P = CaptureParams();
    float BiomeValue = GetBiomeValue(WorldX, WorldY, P.Seed);
    float Height;
    EBlockId S, Sub;
    int32 D;
    GetBlendedBiomeParams(BiomeValue, WorldX, WorldY, P, Height, S, Sub, D);
    return Height;
}
