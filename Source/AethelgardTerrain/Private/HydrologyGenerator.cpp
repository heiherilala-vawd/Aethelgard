#include "AethelgardTerrain/HydrologyGenerator.h"
#include <queue>
#include <vector>

static constexpr int32 D8_DX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static constexpr int32 D8_DY[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
static constexpr float D8_DIST[8] = { 1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f };

struct FFloodNode
{
	int32 Index;
	float Height;
};

struct FFloodNodeGreater
{
	bool operator()(const FFloodNode& A, const FFloodNode& B) const
	{
		return A.Height > B.Height;
	}
};

int32 TerrainHydrology::PriorityFloodFill(
	const TArray<float>& InRawHeight,
	const TArray<bool>& IsOcean,
	int32 GridWidth,
	int32 GridHeight,
	TArray<float>& OutFilledHeight,
	TArray<int32>& OutFillOrder)
{
	const int32 N = GridWidth * GridHeight;
	OutFilledHeight = InRawHeight;
	OutFillOrder.Reset();
	OutFillOrder.Reserve(N);

	TArray<uint8> Closed;
	Closed.SetNumZeroed(N);

	std::priority_queue<FFloodNode, std::vector<FFloodNode>, FFloodNodeGreater> PQ;

	for (int32 Y = 0; Y < GridHeight; Y++)
		for (int32 X = 0; X < GridWidth; X++)
		{
			int32 Idx = Y * GridWidth + X;
			bool bBorder = (X == 0 || X == GridWidth - 1 || Y == 0 || Y == GridHeight - 1);
			if (bBorder || (IsOcean.Num() > 0 && IsOcean[Idx]))
			{
				PQ.push({ Idx, InRawHeight[Idx] });
				Closed[Idx] = 1;
			}
		}

	while (!PQ.empty())
	{
		FFloodNode Cur = PQ.top();
		PQ.pop();

		OutFillOrder.Add(Cur.Index);

		int32 CX = Cur.Index % GridWidth;
		int32 CY = Cur.Index / GridWidth;

		for (int32 d = 0; d < 8; d++)
		{
			int32 NX = CX + D8_DX[d];
			int32 NY = CY + D8_DY[d];
			if (NX < 0 || NX >= GridWidth || NY < 0 || NY >= GridHeight) continue;

			int32 NIdx = NY * GridWidth + NX;
			if (Closed[NIdx]) continue;

			float FillTo = FMath::Max(InRawHeight[NIdx], Cur.Height);

			PQ.push({ NIdx, FillTo });
			Closed[NIdx] = 1;
			OutFilledHeight[NIdx] = FillTo;
		}
	}

	return N;
}

int32 TerrainHydrology::DetectBasins(
	const TArray<float>& RawHeight,
	const TArray<float>& FilledHeight,
	int32 GridWidth,
	int32 GridHeight,
	float MinDepth,
	const FHydrologySettings& Settings,
	TArray<int32>& OutBasinLabels)
{
	const int32 N = GridWidth * GridHeight;
	OutBasinLabels.SetNumUninitialized(N);
	for (int32 i = 0; i < N; i++)
	{
		float Diff = FilledHeight[i] - RawHeight[i];
		OutBasinLabels[i] = (Diff > MinDepth) ? -1 : 0;
	}

	int32 NumBasins = 0;
	TArray<int32> Stack;
	for (int32 i = 0; i < N; i++)
	{
		if (OutBasinLabels[i] != -1) continue;

		NumBasins++;
		Stack.Add(i);
		OutBasinLabels[i] = NumBasins;

		while (Stack.Num() > 0)
		{
			int32 Idx = Stack.Pop();
			int32 CX = Idx % GridWidth;
			int32 CY = Idx / GridWidth;

			for (int32 d = 0; d < 8; d++)
			{
				int32 NX = CX + D8_DX[d];
				int32 NY = CY + D8_DY[d];
				if (NX < 0 || NX >= GridWidth || NY < 0 || NY >= GridHeight) continue;
				int32 NIdx = NY * GridWidth + NX;
				if (OutBasinLabels[NIdx] == -1)
				{
					OutBasinLabels[NIdx] = NumBasins;
					Stack.Add(NIdx);
				}
			}
		}
	}

	TArray<int32> Area;
	Area.SetNumZeroed(NumBasins + 1);
	for (int32 i = 0; i < N; i++)
		if (OutBasinLabels[i] > 0)
			Area[OutBasinLabels[i]]++;

	TArray<int32> Remap;
	Remap.SetNumZeroed(NumBasins + 1);

	int32 Filtered = 0;
	for (int32 b = 1; b <= NumBasins; b++)
		if (Area[b] >= Settings.LakeMinArea)
			Remap[b] = ++Filtered;

	for (int32 i = 0; i < N; i++)
		OutBasinLabels[i] = (OutBasinLabels[i] > 0) ? Remap[OutBasinLabels[i]] : 0;

	return Filtered;
}

void TerrainHydrology::ComputeLakeWater(
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
	TArray<EWaterType>& OutWaterType)
{
	const int32 N = GridWidth * GridHeight;
	OutWaterDepth.SetNumZeroed(N);
	OutWaterSurface.SetNumZeroed(N);
	OutWaterType.SetNumZeroed(N);

	TArray<float> BasinMinRaw;
	TArray<float> BasinMaxFilled;
	TArray<float> BasinAvgHumidity;
	TArray<float> BasinAvgTemp;
	TArray<int32> BasinCount;
	BasinMinRaw.SetNumZeroed(NumBasins + 1);
	BasinMaxFilled.SetNumZeroed(NumBasins + 1);
	BasinAvgHumidity.SetNumZeroed(NumBasins + 1);
	BasinAvgTemp.SetNumZeroed(NumBasins + 1);
	BasinCount.SetNumZeroed(NumBasins + 1);

	for (int32 b = 1; b <= NumBasins; b++)
	{
		BasinMinRaw[b] = MAX_FLT;
		BasinMaxFilled[b] = FLT_MIN;
	}

	for (int32 i = 0; i < N; i++)
	{
		int32 B = BasinLabels[i];
		if (B <= 0 || B > NumBasins) continue;
		float H = RawHeight[i];
		float FH = FilledHeight[i];
		if (H < BasinMinRaw[B]) BasinMinRaw[B] = H;
		if (FH > BasinMaxFilled[B]) BasinMaxFilled[B] = FH;
		BasinAvgHumidity[B] += Humidity.Num() > 0 ? Humidity[i] : 0.5f;
		BasinAvgTemp[B] += Temperature.Num() > 0 ? Temperature[i] : 0.5f;
		BasinCount[B]++;
	}

	for (int32 b = 1; b <= NumBasins; b++)
	{
		if (BasinCount[b] > 0)
		{
			BasinAvgHumidity[b] /= (float)BasinCount[b];
			BasinAvgTemp[b] /= (float)BasinCount[b];
		}

		if (BasinAvgHumidity[b] < Settings.LakeMinHumidity) continue;
		if (BasinAvgTemp[b] > Settings.LakeMaxTemperature && BasinAvgHumidity[b] < Settings.LakeMinHumidity) continue;

		float SpillHeight = BasinMaxFilled[b];
		float WaterLevel = BasinMinRaw[b] + Settings.LakeFillRatio * (SpillHeight - BasinMinRaw[b]);

		EWaterType WType = (BasinAvgTemp[b] < 0.0f) ? EWaterType::Glacier : EWaterType::Lake;

		for (int32 i = 0; i < N; i++)
		{
			if (BasinLabels[i] != b) continue;
			if (RawHeight[i] >= WaterLevel) continue;

			OutWaterDepth[i] = WaterLevel - RawHeight[i];
			OutWaterSurface[i] = WaterLevel;
			OutWaterType[i] = WType;
		}
	}
}

void TerrainHydrology::ComputeD8Flow(
	const TArray<float>& FilledHeight,
	const TArray<int32>& FillOrder,
	int32 GridWidth,
	int32 GridHeight,
	TArray<uint8>& OutFlowDir)
{
	const int32 N = GridWidth * GridHeight;
	OutFlowDir.SetNum(N);
	OutFlowDir.Init(255, N);

	for (int32 i = 0; i < FillOrder.Num(); i++)
	{
		int32 Idx = FillOrder[i];
		int32 CX = Idx % GridWidth;
		int32 CY = Idx / GridWidth;

		uint8 BestDir = 255;
		float BestSlope = -MAX_FLT;
		float MyH = FilledHeight[Idx];

		for (int32 d = 0; d < 8; d++)
		{
			int32 NX = CX + D8_DX[d];
			int32 NY = CY + D8_DY[d];
			if (NX < 0 || NX >= GridWidth || NY < 0 || NY >= GridHeight) continue;

			int32 NIdx = NY * GridWidth + NX;
			float NbH = FilledHeight[NIdx];
			float Slope = (MyH - NbH) / D8_DIST[d];

			if (Slope > BestSlope)
			{
				BestSlope = Slope;
				BestDir = (uint8)d;
			}
			else if (Slope == BestSlope && BestDir != 255)
			{
				if (D8_DIST[d] < D8_DIST[BestDir])
					BestDir = (uint8)d;
			}
		}

		if (BestSlope > 0.0f)
			OutFlowDir[Idx] = BestDir;
	}
}

void TerrainHydrology::AccumulateFlow(
	const TArray<uint8>& FlowDir,
	const TArray<float>& Humidity,
	const TArray<float>& Temperature,
	const TArray<float>& HeightMap,
	int32 GridWidth,
	int32 GridHeight,
	const FHydrologySettings& Settings,
	TArray<int32>& OutFlowAcc)
{
	const int32 N = GridWidth * GridHeight;
	OutFlowAcc.SetNum(N);

	for (int32 i = 0; i < N; i++)
	{
		float Rain = 1.0f;
		float H = Humidity.Num() > 0 ? Humidity[i] : 0.5f;
		float T = Temperature.Num() > 0 ? Temperature[i] : 0.5f;

		if (H < Settings.LakeMinHumidity && T > Settings.LakeMaxTemperature)
			Rain = 0.0f;
		else
			Rain = FMath::Lerp(0.1f, 1.5f, H) * FMath::Max(1.0f - T * 0.6f, 0.05f);

		float Alt = HeightMap.Num() > 0 ? HeightMap[i] : 0.0f;
		float AltNorm = FMath::Clamp(Alt / 220.0f, 0.0f, 1.0f);
		float AltFactor = (AltNorm < Settings.RiverMinAltitude)
			? AltNorm / Settings.RiverMinAltitude
			: 1.0f;
		AltFactor *= (1.0f - AltNorm * Settings.AltitudeWaterReduction);
		Rain *= AltFactor;

		OutFlowAcc[i] = FMath::Max(0, FMath::RoundToInt(Rain * 10.0f));
	}

	TArray<int32> Order;
	Order.Reserve(N);
	for (int32 i = 0; i < N; i++) Order.Add(i);
	Order.Sort([&HeightMap](int32 A, int32 B) { return HeightMap[A] > HeightMap[B]; });

	for (int32 i = 0; i < N; i++)
	{
		int32 Idx = Order[i];
		uint8 D = FlowDir[Idx];
		if (D >= 8) continue;

		int32 CX = Idx % GridWidth;
		int32 CY = Idx / GridWidth;
		int32 NX = CX + D8_DX[D];
		int32 NY = CY + D8_DY[D];
		if (NX < 0 || NX >= GridWidth || NY < 0 || NY >= GridHeight) continue;

		int32 NIdx = NY * GridWidth + NX;
		OutFlowAcc[NIdx] += OutFlowAcc[Idx];
	}
}

void TerrainHydrology::CarveRivers(
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
	TArray<float>& OutRiverWidth)
{
	const int32 N = GridWidth * GridHeight;
	OutHeight = InHeight;
	OutWaterDepth.SetNumZeroed(N);
	OutWaterSurface.SetNumZeroed(N);
	OutWaterType.SetNumZeroed(N);
	OutRiverWidth.SetNumZeroed(N);

	for (int32 i = 0; i < N; i++)
	{
		if (LakeWaterDepth.Num() > 0 && LakeWaterDepth[i] > 0.0f)
		{
			OutWaterDepth[i] = LakeWaterDepth[i];
		}
	}

	for (int32 i = 0; i < N; i++)
	{
		if (LakeWaterDepth.Num() > 0 && LakeWaterDepth[i] > 0.0f) continue;

		float LocalThreshold = Settings.RiverThreshold;

		float H = Humidity.Num() > 0 ? Humidity[i] : 0.5f;
		float T = Temperature.Num() > 0 ? Temperature[i] : 0.5f;
		if (H < Settings.LakeMinHumidity && T > Settings.LakeMaxTemperature)
			LocalThreshold *= 10.0f;
		else
		{
			if (H < Settings.LakeMinHumidity) LocalThreshold *= 3.0f;
			if (T > Settings.LakeMaxTemperature) LocalThreshold *= 2.0f;
		}

		{
			float AltNorm = FMath::Clamp(InHeight[i] / 220.0f, 0.0f, 1.0f);
			if (AltNorm < Settings.RiverMinAltitude)
				LocalThreshold *= 3.0f;
		}

		if ((float)FlowAcc[i] < LocalThreshold) continue;

		float Q = FMath::Loge((float)FlowAcc[i] / LocalThreshold);
		if (Q <= 0.0f) continue;

		float Depth = Settings.RiverBaseDepth * Q;
		float RiverW = 1.0f + Q * Settings.RiverWidthScale;

		if (Temperature.Num() > 0 && Temperature[i] < 0.0f)
		{
			float NewSurface = InHeight[i];
			OutWaterDepth[i] = Depth;
			OutWaterSurface[i] = NewSurface;
			OutWaterType[i] = EWaterType::Glacier;
			OutRiverWidth[i] = RiverW;
		}
		else
		{
			OutHeight[i] = InHeight[i] - Depth;
			float WaterSurface = InHeight[i];
			OutWaterDepth[i] = Depth;
			OutWaterSurface[i] = WaterSurface;
			OutWaterType[i] = EWaterType::River;
			OutRiverWidth[i] = RiverW;
		}
	}
}
