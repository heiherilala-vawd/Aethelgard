#include "AethelgardTerrain/RegionGenerator.h"
#include "AethelgardTerrain/CoordinateUtils.h"
#include "AethelgardTerrain/HydrologyGenerator.h"

FRegionGenerationOutput TerrainRegionGenerator::GenerateRegion(
	const FRegionGenerationInput& Input)
{
	FRegionGenerationOutput Output;
	Output.Key.Coord = Input.RegionCoord;
	Output.Key.GenerationVersion = Input.GenerationVersion;

	const int32 CorePX = TerrainRegion::REGION_PIXELS;
	const int32 SuperPX = TerrainRegion::SUPER_PIXELS;
	const int32 SuperMarginBlocks = TerrainRegion::SUPER_MARGIN_CHUNKS * CHUNK_SIZE;
	const int32 CoreBlockOriginX = Input.RegionCoord.X * TerrainRegion::REGION_BLOCKS;
	const int32 CoreBlockOriginY = Input.RegionCoord.Y * TerrainRegion::REGION_BLOCKS;
	const int32 SuperBlockOriginX = CoreBlockOriginX - SuperMarginBlocks;
	const int32 SuperBlockOriginY = CoreBlockOriginY - SuperMarginBlocks;

	const int32 N = SuperPX * SuperPX;

	FRawRegionMaps RawMaps;
	RawMaps.Allocate(N);

	for (int32 PY = 0; PY < SuperPX; PY++)
		for (int32 PX = 0; PX < SuperPX; PX++)
		{
			int32 Idx = PY * SuperPX + PX;
			int32 WX = SuperBlockOriginX + PX * TerrainRegion::HYDRO_PIXEL_SIZE;
			int32 WY = SuperBlockOriginY + PY * TerrainRegion::HYDRO_PIXEL_SIZE;

			FColumnResult CR = UWorldGeneratorComponent::ComputeRawColumnAt(WX, WY, Input.Params);

			RawMaps.Height[Idx] = CR.Height;
			RawMaps.Temperature[Idx] = CR.Temperature;
			RawMaps.Humidity[Idx] = CR.Humidity;
			RawMaps.Biome[Idx] = CR.Biome;
			RawMaps.IsOcean[Idx] = CR.bIsOcean;
			RawMaps.WaterSurface[Idx] = (float)CR.WaterSurface;
		}

	TArray<float> FilledHeight;
	TArray<int32> FillOrder;
	TerrainHydrology::PriorityFloodFill(RawMaps.Height, RawMaps.IsOcean,
		SuperPX, SuperPX, FilledHeight, FillOrder);

	TArray<int32> BasinLabels;
	int32 NumBasins = TerrainHydrology::DetectBasins(RawMaps.Height, FilledHeight,
		SuperPX, SuperPX, Input.HydroSettings.LakeMinDepth,
		Input.HydroSettings, BasinLabels);

	TArray<float> LakeWaterDepth;
	TArray<float> LakeWaterSurface;
	TArray<EWaterType> LakeWaterType;
	if (NumBasins > 0)
	{
		TerrainHydrology::ComputeLakeWater(RawMaps.Height, FilledHeight,
			BasinLabels, NumBasins, RawMaps.Temperature, RawMaps.Humidity,
			SuperPX, SuperPX, Input.HydroSettings,
			LakeWaterDepth, LakeWaterSurface, LakeWaterType);
	}
	else
	{
		LakeWaterDepth.SetNumZeroed(N);
		LakeWaterSurface.SetNumZeroed(N);
		LakeWaterType.SetNumZeroed(N);
	}

	TArray<uint8> FlowDir;
	TerrainHydrology::ComputeD8Flow(FilledHeight, FillOrder,
		SuperPX, SuperPX, FlowDir);

	TArray<int32> FlowAcc;
	TerrainHydrology::AccumulateFlow(FlowDir, RawMaps.Humidity, RawMaps.Temperature,
		RawMaps.Height, SuperPX, SuperPX, Input.HydroSettings, FlowAcc);

	TArray<float> FinalHeight;
	TArray<float> WaterDepth;
	TArray<float> WaterSurface;
	TArray<EWaterType> WaterType;
	TArray<float> RiverWidth;
	TerrainHydrology::CarveRivers(RawMaps.Height, FlowAcc, FlowDir,
		RawMaps.Temperature, RawMaps.Humidity, LakeWaterDepth,
		SuperPX, SuperPX, Input.HydroSettings,
		FinalHeight, WaterDepth, WaterSurface, WaterType, RiverWidth);

	for (int32 i = 0; i < N; i++)
	{
		if (LakeWaterDepth.Num() > 0 && LakeWaterDepth[i] > 0.0f)
		{
			if (WaterDepth[i] < LakeWaterDepth[i])
			{
				WaterDepth[i] = LakeWaterDepth[i];
				WaterSurface[i] = LakeWaterSurface[i];
				WaterType[i] = LakeWaterType[i];
			}
		}
	}

	int32 MarginPX = TerrainRegion::SUPER_MARGIN_CHUNKS * CHUNK_SIZE / TerrainRegion::HYDRO_PIXEL_SIZE;
	int32 CoreN = CorePX * CorePX;

	Output.PixelWidth = CorePX;
	Output.PixelHeight = CorePX;
	Output.BlockOrigin = FIntPoint(CoreBlockOriginX, CoreBlockOriginY);

	Output.RawHeight.SetNum(CoreN);
	Output.FinalHeight.SetNum(CoreN);
	Output.Temperature.SetNum(CoreN);
	Output.Humidity.SetNum(CoreN);
	Output.Biome.SetNum(CoreN);
	Output.IsOcean.SetNum(CoreN);
	Output.WaterDepth.SetNum(CoreN);
	Output.WaterSurface.SetNum(CoreN);
	Output.WaterType.SetNum(CoreN);
	Output.RiverWidth.SetNum(CoreN);

	for (int32 PY = 0; PY < CorePX; PY++)
		for (int32 PX = 0; PX < CorePX; PX++)
		{
			int32 SrcIdx = (MarginPX + PY) * SuperPX + (MarginPX + PX);
			int32 DstIdx = PY * CorePX + PX;

			Output.RawHeight[DstIdx] = RawMaps.Height[SrcIdx];
			Output.FinalHeight[DstIdx] = FinalHeight[SrcIdx];
			Output.Temperature[DstIdx] = RawMaps.Temperature[SrcIdx];
			Output.Humidity[DstIdx] = RawMaps.Humidity[SrcIdx];
			Output.Biome[DstIdx] = RawMaps.Biome[SrcIdx];
			Output.IsOcean[DstIdx] = RawMaps.IsOcean[SrcIdx];
			Output.WaterDepth[DstIdx] = WaterDepth[SrcIdx];
			Output.WaterSurface[DstIdx] = WaterSurface[SrcIdx];
			Output.WaterType[DstIdx] = WaterType[SrcIdx];
			Output.RiverWidth[DstIdx] = RiverWidth[SrcIdx];
		}

	RawMaps.Free();

	return Output;
}
