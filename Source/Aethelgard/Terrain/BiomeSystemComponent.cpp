// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/BiomeSystemComponent.h"

UBiomeSystemComponent::UBiomeSystemComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    FBiomeDefinition Desert;
    Desert.Id = EBiomeId::Desert;
    Desert.Name = TEXT("Desert");
    Desert.MinTemperature = 0.3f;
    Desert.MaxTemperature = 2.0f;
    Desert.MinHumidity = -2.0f;
    Desert.MaxHumidity = -0.1f;
    Desert.SurfaceBlock = EBlockId::Sand;
    Desert.SubSurfaceBlock = EBlockId::Sand;
    Desert.DeepBlock = EBlockId::Stone;
    Desert.HeightMultiplier = 0.6f;
    BiomeDefinitions.Add(Desert);

    FBiomeDefinition Plains;
    Plains.Id = EBiomeId::Plains;
    Plains.Name = TEXT("Plains");
    Plains.MinTemperature = -0.3f;
    Plains.MaxTemperature = 0.5f;
    Plains.MinHumidity = -0.3f;
    Plains.MaxHumidity = 0.3f;
    Plains.SurfaceBlock = EBlockId::Grass;
    Plains.SubSurfaceBlock = EBlockId::Dirt;
    Plains.DeepBlock = EBlockId::Stone;
    Plains.HeightMultiplier = 1.0f;
    BiomeDefinitions.Add(Plains);

    FBiomeDefinition Forest;
    Forest.Id = EBiomeId::Forest;
    Forest.Name = TEXT("Forest");
    Forest.MinTemperature = -0.2f;
    Forest.MaxTemperature = 0.6f;
    Forest.MinHumidity = 0.3f;
    Forest.MaxHumidity = 2.0f;
    Forest.SurfaceBlock = EBlockId::Grass;
    Forest.SubSurfaceBlock = EBlockId::Dirt;
    Forest.DeepBlock = EBlockId::Stone;
    Forest.HeightMultiplier = 1.3f;
    BiomeDefinitions.Add(Forest);

    FBiomeDefinition Ocean;
    Ocean.Id = EBiomeId::Ocean;
    Ocean.Name = TEXT("Ocean");
    Ocean.MinTemperature = -2.0f;
    Ocean.MaxTemperature = 2.0f;
    Ocean.MinHumidity = -2.0f;
    Ocean.MaxHumidity = 2.0f;
    Ocean.SurfaceBlock = EBlockId::Sand;
    Ocean.SubSurfaceBlock = EBlockId::Sand;
    Ocean.DeepBlock = EBlockId::Stone;
    Ocean.HeightMultiplier = 0.3f;
    BiomeDefinitions.Add(Ocean);
}

EBiomeId UBiomeSystemComponent::GetBiome(int32 WorldX, int32 WorldY) const
{
    float Temp = GetTemperature((float)WorldX, (float)WorldY);
    float Hum = GetHumidity((float)WorldX, (float)WorldY);

    EBiomeId BestBiome = EBiomeId::Plains;
    float BestScore = -1.0f;

    for (const FBiomeDefinition& Biome : BiomeDefinitions)
    {
        if (Biome.Id == EBiomeId::Ocean)
            continue;

        if (Temp >= Biome.MinTemperature && Temp <= Biome.MaxTemperature &&
            Hum >= Biome.MinHumidity && Hum <= Biome.MaxHumidity)
        {
            float CenterT = (Biome.MinTemperature + Biome.MaxTemperature) * 0.5f;
            float CenterH = (Biome.MinHumidity + Biome.MaxHumidity) * 0.5f;
            float Dist = FMath::Square(Temp - CenterT) + FMath::Square(Hum - CenterH);
            float Score = 1.0f / (1.0f + Dist);

            if (Score > BestScore)
            {
                BestScore = Score;
                BestBiome = Biome.Id;
            }
        }
    }

    return BestBiome;
}

const FBiomeDefinition& UBiomeSystemComponent::GetBiomeDefinition(EBiomeId Biome) const
{
    for (const FBiomeDefinition& Def : BiomeDefinitions)
    {
        if (Def.Id == Biome)
            return Def;
    }
    return BiomeDefinitions[0];
}

EBlockId UBiomeSystemComponent::GetSurfaceBlock(int32 WorldX, int32 WorldY) const
{
    return GetBiomeDefinition(GetBiome(WorldX, WorldY)).SurfaceBlock;
}

EBlockId UBiomeSystemComponent::GetSubSurfaceBlock(int32 WorldX, int32 WorldY) const
{
    return GetBiomeDefinition(GetBiome(WorldX, WorldY)).SubSurfaceBlock;
}

EBlockId UBiomeSystemComponent::GetDeepBlock(int32 WorldX, int32 WorldY) const
{
    return GetBiomeDefinition(GetBiome(WorldX, WorldY)).DeepBlock;
}

float UBiomeSystemComponent::GetHeightMultiplier(int32 WorldX, int32 WorldY) const
{
    return GetBiomeDefinition(GetBiome(WorldX, WorldY)).HeightMultiplier;
}

float UBiomeSystemComponent::GetBaseHeight(int32 WorldX, int32 WorldY) const
{
    EBiomeId Biome = GetBiome(WorldX, WorldY);
    if (Biome == EBiomeId::Desert)    return 42.0f;
    if (Biome == EBiomeId::Plains)    return 35.0f;
    if (Biome == EBiomeId::Forest)    return 38.0f;
    if (Biome == EBiomeId::Ocean)     return 25.0f;
    return 35.0f;
}

float UBiomeSystemComponent::GetTemperature(float WorldX, float WorldY) const
{
    return GetNoise2D(WorldX, WorldY, 0.0008f, 3, Seed + 1000);
}

float UBiomeSystemComponent::GetHumidity(float WorldX, float WorldY) const
{
    return GetNoise2D(WorldX, WorldY, 0.0008f, 3, Seed + 2000);
}

float UBiomeSystemComponent::GetNoise2D(float X, float Y, float Scale, int32 OctaveCount, int32 NoiseSeed) const
{
    float Value = 0.0f;
    float Amplitude = 1.0f;
    float MaxAmplitude = 0.0f;
    float Freq = 1.0f;

    for (int32 i = 0; i < OctaveCount; i++)
    {
        FRandomStream Stream(NoiseSeed + i * 7919);
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
