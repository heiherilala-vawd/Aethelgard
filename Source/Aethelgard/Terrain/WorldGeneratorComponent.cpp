// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/WorldGeneratorComponent.h"

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData)
{
    const FIntVector& CP = ChunkData.Position;

    for (int32 Z = 0; Z < CHUNK_SIZE; Z++)
    {
        for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
        {
            for (int32 X = 0; X < CHUNK_SIZE; X++)
            {
                int32 WX = CP.X * CHUNK_SIZE + X;
                int32 WY = CP.Y * CHUNK_SIZE + Y;
                int32 WZ = CP.Z * CHUNK_SIZE + Z;

                EBiomeId Biome = EBiomeId::Plains;
                float Height = GetHeight(WX, WY);
                bool bIsOcean = false;

                if (BiomeSystem)
                {
                    Biome = BiomeSystem->GetBiome(WX, WY);
                    if (Biome == EBiomeId::Ocean)
                        bIsOcean = true;
                }

                EBlockId Block = EBlockId::Air;

                if (WZ < Height)
                {
                    int32 Depth = FMath::FloorToInt(Height) - WZ;

                    if (BiomeSystem)
                    {
                        if (Depth <= 0)
                            Block = BiomeSystem->GetSurfaceBlock(WX, WY);
                        else if (Depth <= StoneDepth)
                            Block = BiomeSystem->GetSubSurfaceBlock(WX, WY);
                        else
                            Block = BiomeSystem->GetDeepBlock(WX, WY);
                    }
                    else
                    {
                        if (Depth <= 0)
                            Block = EBlockId::Grass;
                        else if (Depth <= StoneDepth)
                            Block = EBlockId::Dirt;
                        else
                            Block = EBlockId::Stone;
                    }
                }
                else if (WZ <= FMath::FloorToInt(WaterLevel))
                {
                    Block = bIsOcean ? EBlockId::Water : EBlockId::Water;
                }

                ChunkData.SetBlock(X, Y, Z, Block);
            }
        }
    }

    if (BiomeSystem)
    {
        TArray<FIntVector> TreePositions = GetTreePositions(CP.X, CP.Y);
        for (const FIntVector& TreePos : TreePositions)
        {
            int32 TX = TreePos.X;
            int32 TY = TreePos.Y;
            if (TX < 0 || TX >= CHUNK_SIZE || TY < 0 || TY >= CHUNK_SIZE)
                continue;

            int32 WX = CP.X * CHUNK_SIZE + TX;
            int32 WY = CP.Y * CHUNK_SIZE + TY;
            EBiomeId Biome = BiomeSystem->GetBiome(WX, WY);

            if (Biome != EBiomeId::Forest)
                continue;

            float H = GetHeight(WX, WY);
            int32 SurfaceZ = FMath::FloorToInt(H) - CP.Z * CHUNK_SIZE;

            if (SurfaceZ >= 0 && SurfaceZ < CHUNK_SIZE - 4)
            {
                for (int32 TreeH = 1; TreeH <= 4; TreeH++)
                {
                    ChunkData.SetBlock(TX, TY, SurfaceZ + TreeH, EBlockId::Wood);
                }
                for (int32 DX = -1; DX <= 1; DX++)
                {
                    for (int32 DY = -1; DY <= 1; DY++)
                    {
                        for (int32 DZ = 0; DZ <= 2; DZ++)
                        {
                            int32 LX = TX + DX;
                            int32 LY = TY + DY;
                            int32 LZ = SurfaceZ + 5 + DZ;
                            if (LX >= 0 && LX < CHUNK_SIZE && LY >= 0 && LY < CHUNK_SIZE && LZ >= 0 && LZ < CHUNK_SIZE)
                            {
                                if (DX != 0 || DY != 0 || DZ != 1)
                                {
                                    ChunkData.SetBlock(LX, LY, LZ, EBlockId::Leaves);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ChunkData.bIsGenerated = true;
}

float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const
{
    float Noise = GetNoise2D((float)WorldX, (float)WorldY, NoiseScale, Octaves);

    float BiomeBase = BaseHeight;
    float BiomeMultiplier = 1.0f;

    if (BiomeSystem)
    {
        BiomeBase = BiomeSystem->GetBaseHeight(WorldX, WorldY);
        BiomeMultiplier = BiomeSystem->GetHeightMultiplier(WorldX, WorldY);
    }

    return BiomeBase + Noise * HeightScale * BiomeMultiplier;
}

TArray<FIntVector> UWorldGeneratorComponent::GetTreePositions(int32 ChunkX, int32 ChunkY) const
{
    TArray<FIntVector> Positions;

    FRandomStream RNG(Seed + ChunkX * 7919 + ChunkY * 6271);
    int32 TreeCount = RNG.RandRange(0, 4);

    for (int32 i = 0; i < TreeCount; i++)
    {
        int32 TX = RNG.RandRange(2, CHUNK_SIZE - 3);
        int32 TY = RNG.RandRange(2, CHUNK_SIZE - 3);
        Positions.Add(FIntVector(TX, TY, 0));
    }

    return Positions;
}

float UWorldGeneratorComponent::GetNoise2D(float X, float Y, float Scale, int32 OctaveCount) const
{
    float Value = 0.0f;
    float Amplitude = 1.0f;
    float MaxAmplitude = 0.0f;
    float Freq = 1.0f;

    for (int32 i = 0; i < OctaveCount; i++)
    {
        FRandomStream Stream(Seed + i * 7919);
        float OffsetX = Stream.FRand() * 10000.0f;
        float OffsetY = Stream.FRand() * 10000.0f;

        Value += Amplitude * FMath::PerlinNoise2D(FVector2D(
            X * Scale * Freq + OffsetX,
            Y * Scale * Freq + OffsetY
        ));

        MaxAmplitude += Amplitude;
        Amplitude *= 0.5f;
        Freq *= 2.0f;
    }

    return Value / MaxAmplitude;
}
