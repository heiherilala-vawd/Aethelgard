// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/WorldGeneratorComponent.h"

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData)
{
    const FIntPoint& CP = ChunkData.Position;

    for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
    {
        for (int32 X = 0; X < CHUNK_SIZE; X++)
        {
            int32 WX = CP.X * CHUNK_SIZE + X;
            int32 WY = CP.Y * CHUNK_SIZE + Y;
            float Height = GetHeight(WX, WY);

            for (int32 Z = 0; Z < WORLD_HEIGHT; Z++)
            {
                EBlockId Block;
                if ((float)Z < Height - 4.0f)
                    Block = EBlockId::Stone;
                else if ((float)Z < Height - 1.0f)
                    Block = EBlockId::Dirt;
                else if ((float)Z < Height)
                    Block = EBlockId::Grass;
                else if ((float)Z <= WaterLevel)
                    Block = EBlockId::Water;
                else
                    Block = EBlockId::Air;

                ChunkData.SetBlock(X, Y, Z, Block);
            }
        }
    }

    ChunkData.bIsGenerated = true;
}

float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const
{
    float Value = 0.0f, Amplitude = 1.0f, MaxAmp = 0.0f, Freq = 1.0f;
    for (int32 i = 0; i < Octaves; i++)
    {
        FRandomStream Stream(Seed + i * 7919);
        float OX = Stream.FRand() * 10000.0f, OY = Stream.FRand() * 10000.0f;
        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(
            (float)WorldX * NoiseScale * Freq + OX,
            (float)WorldY * NoiseScale * Freq + OY));
        MaxAmp += Amplitude;
        Amplitude *= 0.5f;
        Freq *= 2.0f;
    }
    return BaseHeight + (Value / MaxAmp) * HeightScale;
}
