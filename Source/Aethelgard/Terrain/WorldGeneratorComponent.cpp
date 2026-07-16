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

static float GetHeightAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
    float Noise = GetNoise2D((float)WX, (float)WY, P.NoiseScale, P.Octaves, P.Seed);
    return P.BaseHeight + Noise * P.HeightScale;
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
            float Height = GetHeightAt(WX, WY, P);

            for (int32 Z = 0; Z < WORLD_HEIGHT; Z++)
            {
                EBlockId Block;
                if ((float)Z < Height - 4.0f)      Block = EBlockId::Stone;
                else if ((float)Z < Height - 1.0f) Block = EBlockId::Dirt;
                else if ((float)Z < Height)        Block = EBlockId::Grass;
                else if ((float)Z <= P.WaterLevel) Block = EBlockId::Water;
                else                               Block = EBlockId::Air;

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
    return GetHeightAt(WorldX, WorldY, CaptureParams());
}
