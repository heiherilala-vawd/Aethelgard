// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/WorldGeneratorComponent.h"

static constexpr float BiomeCenters[4] = { 0.125f, 0.375f, 0.625f, 0.875f };

static const FBiomeParams BiomeParams[4] = {
    { 44.0f, 10.0f, 0.010f, 3, EBlockId::Grass,  EBlockId::Dirt,  3 },
    { 48.0f, 12.0f, 0.007f, 2, EBlockId::Sand,   EBlockId::Sand,  5 },
    { 60.0f, 35.0f, 0.005f, 4, EBlockId::Grass,  EBlockId::Stone, 2 },
    { 50.0f, 18.0f, 0.006f, 3, EBlockId::Grass,  EBlockId::Dirt,  4 },
};

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

float UWorldGeneratorComponent::GetBiomeValue(int32 WX, int32 WY, int32 Seed)
{
    float Noise = GetNoise2D((float)WX, (float)WY, 0.0000375f, 2,
                             Seed + (int32)ENoiseLayer::NBlend * 7919);
    return FMath::Fmod(FMath::Fmod(Noise, 1.0f) + 1.0f, 1.0f);
}

FColumnHeightInfo UWorldGeneratorComponent::ComputeHeightAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
    float BV = GetBiomeValue(WX, WY, P.Seed);

    float Heights[4];
    for (int32 i = 0; i < 4; i++)
    {
        Heights[i] = GetNoise2D((float)WX, (float)WY, BiomeParams[i].NoiseScale,
                                 BiomeParams[i].Octaves,
                                 P.Seed + ((int32)ENoiseLayer::NBiome1 + i) * 7919);
        Heights[i] = BiomeParams[i].BaseHeight + Heights[i] * BiomeParams[i].HeightScale;
    }

    float Weights[4];
    float TotalWeight = 0.0f;
    constexpr float Sharpness = 30.0f;
    int32 DominantIdx = 0;
    float MaxW = 0.0f;

    for (int32 i = 0; i < 4; i++)
    {
        float d = FMath::Abs(BV - BiomeCenters[i]);
        float dWrap = FMath::Min(d, 1.0f - d);
        Weights[i] = FMath::Exp(-dWrap * dWrap * Sharpness);
        TotalWeight += Weights[i];
        if (Weights[i] > MaxW) { MaxW = Weights[i]; DominantIdx = i; }
    }

    float Result = 0.0f;
    for (int32 i = 0; i < 4; i++)
        Result += (Weights[i] / TotalWeight) * Heights[i];

    return { Result, DominantIdx };
}

void UWorldGeneratorComponent::GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P)
{
    const FIntPoint& CP = ChunkData.Position;
    const int32 SX = CP.X * CHUNK_SIZE;
    const int32 SY = CP.Y * CHUNK_SIZE;

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
    {
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            int32 WX = SX + X;
            int32 WY = SY + Y;

            FColumnHeightInfo Info = ComputeHeightAt(WX, WY, P);
            float Height = Info.Height;
            int32 DominantIdx = Info.DominantIdx;

            EBlockId SurfaceBlock = BiomeParams[DominantIdx].SurfaceBlock;
            EBlockId SubsurfaceBlock = BiomeParams[DominantIdx].SubsurfaceBlock;
            int32 SubsurfaceDepth = BiomeParams[DominantIdx].SubsurfaceDepth;

            if (DominantIdx == 2 && Height > 75.0f)
            {
                SurfaceBlock = EBlockId::Stone;
                SubsurfaceBlock = EBlockId::Stone;
                SubsurfaceDepth = 1;
            }

            float N1 = GetNoise2D((float)WX, (float)WY, 0.01f, 1,
                                   P.Seed + (int32)ENoiseLayer::NLayer1 * 7919);
            int32 Perturb1 = (N1 > 0.3f) ? 1 : (N1 < -0.3f) ? -1 : 0;

            float N2 = GetNoise2D((float)WX, (float)WY, 0.01f, 1,
                                   P.Seed + (int32)ENoiseLayer::NLayer2 * 7919);
            int32 Perturb2 = (N2 > 0.3f) ? 1 : (N2 < -0.3f) ? -1 : 0;

            int32 StoneBoundary = (int32)(Height - SubsurfaceDepth) + Perturb1;
            int32 SubBoundary = (int32)(Height - 1) + Perturb2;

            StoneBoundary = FMath::Clamp(StoneBoundary, 0, WORLD_HEIGHT - 1);
            SubBoundary = FMath::Clamp(SubBoundary, StoneBoundary + 1, WORLD_HEIGHT);

            uint8 Top[TOP_LAYERS];
            uint8 EH = (uint8)Height;

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

            ChunkData.SetColumn(X, Y, EH, Top);
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
    return ComputeHeightAt(WorldX, WorldY, P).Height;
}
