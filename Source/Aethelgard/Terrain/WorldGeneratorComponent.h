// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ChunkData.h"
#include "Terrain/BiomeSystemComponent.h"
#include "WorldGeneratorComponent.generated.h"

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class UWorldGeneratorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "World")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float HeightScale = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float BaseHeight = 40.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float NoiseScale = 0.005f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    int32 Octaves = 4;

    UPROPERTY(EditAnywhere, Category = "Generation")
    float WaterLevel = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Generation")
    int32 StoneDepth = 4;

    void SetBiomeSystem(UBiomeSystemComponent* InBiomeSystem) { BiomeSystem = InBiomeSystem; }

    void GenerateChunk(FChunkData& ChunkData);

    float GetHeight(int32 WorldX, int32 WorldY) const;

    TArray<FIntVector> GetTreePositions(int32 ChunkX, int32 ChunkY) const;

private:
    UPROPERTY()
    UBiomeSystemComponent* BiomeSystem = nullptr;

    float GetNoise2D(float X, float Y, float Scale, int32 OctaveCount) const;
};
