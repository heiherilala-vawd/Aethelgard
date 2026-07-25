#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/TerrainRegionData.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"

struct AETHELGARDTERRAIN_API FRegionGenerationInput
{
	FIntPoint RegionCoord;
	int32 Seed;
	int32 GenerationVersion = 0;
	FGeneratorParams Params;
	FHydrologySettings HydroSettings;
};

struct AETHELGARDTERRAIN_API FRegionGenerationOutput
{
	FTerrainRegionKey Key;

	TArray<float> RawHeight;
	TArray<float> FinalHeight;
	TArray<float> Temperature;
	TArray<float> Humidity;
	TArray<EBiomeType> Biome;
	TArray<bool> IsOcean;

	TArray<float> WaterDepth;
	TArray<float> WaterSurface;
	TArray<EWaterType> WaterType;
	TArray<float> RiverWidth;

	int32 PixelWidth = 0;
	int32 PixelHeight = 0;
	FIntPoint BlockOrigin;
};

namespace TerrainRegionGenerator
{
	FRegionGenerationOutput GenerateRegion(
		const FRegionGenerationInput& Input);
}
