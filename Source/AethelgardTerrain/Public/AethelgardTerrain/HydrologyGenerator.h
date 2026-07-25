#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/TerrainRegionData.h"

namespace TerrainHydrology
{
	int32 PriorityFloodFill(
		const TArray<float>& InRawHeight,
		const TArray<bool>& IsOcean,
		int32 GridWidth,
		int32 GridHeight,
		TArray<float>& OutFilledHeight,
		TArray<int32>& OutFillOrder);

	int32 DetectBasins(
		const TArray<float>& RawHeight,
		const TArray<float>& FilledHeight,
		int32 GridWidth,
		int32 GridHeight,
		float MinDepth,
		const FHydrologySettings& Settings,
		TArray<int32>& OutBasinLabels);

	void ComputeLakeWater(
		const TArray<float>& RawHeight,
		const TArray<float>& FilledHeight,
		const TArray<int32>& BasinLabels,
		int32 NumBasins,
		const TArray<float>& Temperature,
		const TArray<float>& Humidity,
		int32 GridWidth,
		int32 GridHeight,
		const FHydrologySettings& Settings,
		TArray<float>& OutWaterDepth,
		TArray<float>& OutWaterSurface,
		TArray<EWaterType>& OutWaterType);

	void ComputeD8Flow(
		const TArray<float>& FilledHeight,
		const TArray<int32>& FillOrder,
		int32 GridWidth,
		int32 GridHeight,
		TArray<uint8>& OutFlowDir);

	void AccumulateFlow(
		const TArray<uint8>& FlowDir,
		const TArray<float>& Humidity,
		const TArray<float>& Temperature,
		const TArray<float>& HeightMap,
		int32 GridWidth,
		int32 GridHeight,
		const FHydrologySettings& Settings,
		TArray<int32>& OutFlowAcc);

	void CarveRivers(
		const TArray<float>& InHeight,
		const TArray<int32>& FlowAcc,
		const TArray<uint8>& FlowDir,
		const TArray<float>& Temperature,
		const TArray<float>& Humidity,
		const TArray<float>& LakeWaterDepth,
		int32 GridWidth,
		int32 GridHeight,
		const FHydrologySettings& Settings,
		TArray<float>& OutHeight,
		TArray<float>& OutWaterDepth,
		TArray<float>& OutWaterSurface,
		TArray<EWaterType>& OutWaterType,
		TArray<float>& OutRiverWidth);
}
