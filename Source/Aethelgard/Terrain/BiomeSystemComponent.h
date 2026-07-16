// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/BlockRegistry.h"
#include "BiomeSystemComponent.generated.h"

UENUM()
enum class EBiomeId : uint8
{
    Desert = 0,
    Plains,
    Forest,
    Ocean,
    MAX UMETA(Hidden)
};

USTRUCT()
struct FBiomeDefinition
{
    GENERATED_BODY()

    EBiomeId Id;
    FName Name;
    float MinTemperature;
    float MaxTemperature;
    float MinHumidity;
    float MaxHumidity;
    EBlockId SurfaceBlock;
    EBlockId SubSurfaceBlock;
    EBlockId DeepBlock;
    float HeightMultiplier;
};

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class UBiomeSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBiomeSystemComponent();

    void SetSeed(int32 InSeed) { Seed = InSeed; }

    EBiomeId GetBiome(int32 WorldX, int32 WorldY) const;
    const FBiomeDefinition& GetBiomeDefinition(EBiomeId Biome) const;

    EBlockId GetSurfaceBlock(int32 WorldX, int32 WorldY) const;
    EBlockId GetSubSurfaceBlock(int32 WorldX, int32 WorldY) const;
    EBlockId GetDeepBlock(int32 WorldX, int32 WorldY) const;
    float GetHeightMultiplier(int32 WorldX, int32 WorldY) const;
    float GetBaseHeight(int32 WorldX, int32 WorldY) const;

private:
    int32 Seed = 0;

    TArray<FBiomeDefinition> BiomeDefinitions;

    float GetTemperature(float WorldX, float WorldY) const;
    float GetHumidity(float WorldX, float WorldY) const;
    float GetNoise2D(float X, float Y, float Scale, int32 Octaves, int32 NoiseSeed) const;
};
