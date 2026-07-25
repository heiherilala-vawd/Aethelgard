#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/ChunkData.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "TerrainRegionData.generated.h"

namespace TerrainRegion
{
	constexpr int32 REGION_CHUNKS = 16;
	constexpr int32 REGION_BLOCKS = REGION_CHUNKS * CHUNK_SIZE;
	constexpr int32 HYDRO_PIXEL_SIZE = 2;
	constexpr int32 REGION_PIXELS = REGION_BLOCKS / HYDRO_PIXEL_SIZE;

	constexpr int32 SUPER_MARGIN_CHUNKS = 8;
	constexpr int32 SUPER_CHUNKS = REGION_CHUNKS + 2 * SUPER_MARGIN_CHUNKS;
	constexpr int32 SUPER_BLOCKS = SUPER_CHUNKS * CHUNK_SIZE;
	constexpr int32 SUPER_PIXELS = SUPER_BLOCKS / HYDRO_PIXEL_SIZE;
}

UENUM()
enum class EWaterType : uint8
{
	None = 0,
	Ocean,
	Lake,
	River,
	Glacier,
	MAX UMETA(Hidden)
};

struct AETHELGARDTERRAIN_API FTerrainRegionKey
{
	FIntPoint Coord;
	int32 GenerationVersion = 0;

	bool operator==(const FTerrainRegionKey& Other) const
	{
		return Coord == Other.Coord && GenerationVersion == Other.GenerationVersion;
	}
};

FORCEINLINE uint32 GetTypeHash(const FTerrainRegionKey& Key)
{
	return HashCombine(GetTypeHash(Key.Coord), GetTypeHash(Key.GenerationVersion));
}

struct AETHELGARDTERRAIN_API FRawRegionMaps
{
	TArray<float> Height;
	TArray<float> Temperature;
	TArray<float> Humidity;
	TArray<EBiomeType> Biome;
	TArray<bool> IsOcean;
	TArray<float> WaterSurface;

	void Allocate(int32 Count)
	{
		Height.SetNum(Count);
		Temperature.SetNum(Count);
		Humidity.SetNum(Count);
		Biome.SetNum(Count);
		IsOcean.SetNum(Count);
		WaterSurface.SetNum(Count);
	}

	void Free()
	{
		Height.Empty();
		Temperature.Empty();
		Humidity.Empty();
		Biome.Empty();
		IsOcean.Empty();
		WaterSurface.Empty();
	}
};

struct AETHELGARDTERRAIN_API FHydrologySettings
{
	float LakeMinDepth = 2.0f;
	float LakeFillRatio = 0.8f;
	int32 LakeMinArea = 40;
	float LakeMinHumidity = 0.35f;
	float LakeMaxTemperature = 0.65f;

	float RiverThreshold = 2000.0f;
	float RiverBaseDepth = 1.5f;
	float RiverWidthScale = 3.0f;
	float RiverMinAltitude = 0.15f;

	float AltitudeWaterReduction = 0.7f;

	int32 SuperMarginChunks = TerrainRegion::SUPER_MARGIN_CHUNKS;
};
