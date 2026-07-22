// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/GenerationDefaults.h"
#include "AethelgardTerrain/NoiseSampler.h"

static float ScoreBiome(float T, float H, float WorldHeight, const FGeneratorParams& P,
	float TempAff, float HumidAff, float HeightAff, float Adjust)
{
	float NH = FMath::Clamp((WorldHeight - 1.0f) / (P.MaxHeight - 1.0f), 0.0f, 1.0f);
	float Dist = P.TempWeight * FMath::Abs(T - TempAff)
			   + P.HumidWeight * FMath::Abs(H - HumidAff)
			   + P.HeightWeight * FMath::Abs(NH - HeightAff);
	return FMath::Exp(-Dist * Dist * P.AffinitySharpness) * Adjust;
}

static const FBiomeParams BiomeParams[5] = {
	{ EBlockId::Grass, EBlockId::Dirt, 3, 0.0f, 0.0f, GenDef::PlainsHillAmplitude, GenDef::PlainsHillScale, true, false },
	{ EBlockId::Sand,  EBlockId::Sand, 5, 0.0f, 0.0f, GenDef::DesertDuneAmplitude, GenDef::DesertDuneScale, false, true },
	{ EBlockId::Grass, EBlockId::Stone, 2, GenDef::MountainRockThreshold, GenDef::MountainSnowThreshold, GenDef::MountainDetailAmplitude, GenDef::MountainDetailScale, false, false },
	{ EBlockId::Grass, EBlockId::Dirt, 4, 0.0f, 0.0f, 0.0f, 0.0f, false, false },
	{ EBlockId::Snow,  EBlockId::Dirt, 3, 0.0f, 0.0f, 0.0f, 0.0f, false, false },
};

static void ComputeClimate(int32 WX, int32 WY, const FGeneratorParams& P, float& OutTemp, float& OutHumid)
{
	float Temp = FMath::Clamp((TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NTemperature * 7919,
		P.TempScale, 1, 0.5f, 2.0f, WX, WY) + 1.0f) * 0.5f, 0.0f, 1.0f);
	float Humid = FMath::Clamp((TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NHumidity * 7919,
		P.HumidScale, 1, 0.5f, 2.0f, WX, WY) + 1.0f) * 0.5f, 0.0f, 1.0f);
	Temp = FMath::Clamp(Temp + TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NTempPerturb * 7919,
		P.TempPerturbScale, 1, 0.5f, 2.0f, WX, WY) * P.TempPerturbAmplitude, 0.0f, 1.0f);
	Humid = FMath::Clamp(Humid + TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NHumidPerturb * 7919,
		P.HumidPerturbScale, 1, 0.5f, 2.0f, WX, WY) * P.HumidPerturbAmplitude, 0.0f, 1.0f);

	OutTemp = Temp;
	OutHumid = Humid;
}

float UWorldGeneratorComponent::ComputeBaseHeight(int32 WX, int32 WY, const FGeneratorParams& P)
{
	float Macro = TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NMacro * 7919,
		P.MacroScale, P.MacroOctaves, P.MacroPersistence, P.MacroLacunarity,
		WX, WY);
	float BaseShape = FMath::Abs(TerrainNoise::Sample2D(
		P.Seed + (int32)ENoiseLayer::NBaseShape * 7919,
		P.BaseShapeScale, 1, P.BaseShapePersistence, P.BaseShapeLacunarity,
		WX, WY));
	return P.GlobalElevation + Macro * P.MacroAmplitude + BaseShape * P.BaseShapeAmplitude;
}

EBiomeType UWorldGeneratorComponent::SelectBiome(int32 WX, int32 WY, const FGeneratorParams& P, float& OutGradient)
{
	float Height = FMath::Clamp(ComputeBaseHeight(WX, WY, P), 1.0f, P.MaxHeight);
	if (Height < P.SeaLevel) { OutGradient = 0.0f; return EBiomeType::Plains; }
	if (Height >= P.MountainStart) { OutGradient = FMath::Clamp((Height - P.MountainStart) / P.BiomeBlendDistance, 0.0f, 1.0f); return EBiomeType::Mountain; }

	float Temp, Humid;
	ComputeClimate(WX, WY, P, Temp, Humid);

	float EffGlacier = FMath::Lerp(P.GlacierThreshold, 1.0f, P.IceAgeFactor);
	EBiomeType Biome;
	if (Temp < EffGlacier) Biome = EBiomeType::Glacier;
	else
	{
		float FS = ScoreBiome(Temp, Humid, Height, P, P.ForestTempAffinity, P.ForestHumidAffinity, P.ForestHeightAffinity, P.ForestAdjust);
		float DS = ScoreBiome(Temp, Humid, Height, P, P.DesertTempAffinity, P.DesertHumidAffinity, P.DesertHeightAffinity, P.DesertAdjust);
		float PS = ScoreBiome(Temp, Humid, Height, P, P.PlainsTempAffinity, P.PlainsHumidAffinity, P.PlainsHeightAffinity, P.PlainsAdjust);
		if (FS >= DS && FS >= PS) Biome = EBiomeType::Forest;
		else if (DS >= PS) Biome = EBiomeType::Desert;
		else Biome = EBiomeType::Plains;
	}

	float dSea = Height - P.SeaLevel, dMtn = P.MountainStart - Height, dGlac = FMath::Abs(Temp - EffGlacier);
	OutGradient = FMath::Clamp(FMath::Min3(dSea, dMtn, dGlac) / P.BiomeBlendDistance, 0.0f, 1.0f);
	return Biome;
}

static float ComputeFollyContribution(int32 WX, int32 WY, float Scale, float Amplitude,
	float Bias, float t, int32 Seed)
{
	float Raw = TerrainNoise::Sample2D(Seed, Scale, 1, 0.5f, 2.0f, WX, WY);
	float N = (Raw + 1.0f) * 0.5f;
	float M = FMath::Max(N - Bias, 0.0f);
	return M * Amplitude * t;
}

FColumnResult UWorldGeneratorComponent::ComputeRawColumnAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
	FColumnResult Result;
	int32 S = P.Seed;

	float Height = FMath::Clamp(ComputeBaseHeight(WX, WY, P), 1.0f, P.MaxHeight);

	if (Height >= P.SeaLevel)
	{
		float t = FMath::Clamp((Height - P.SeaLevel) / 20.0f, 0.0f, 1.0f);
		int32 FS = S + (int32)ENoiseLayer::NMountainFolly * 7919;
		float C1 = ComputeFollyContribution(WX, WY, P.MountainFollyScale, P.MountainFollyAmplitude, P.MountainFollyBias, t, FS);
		float C2 = ComputeFollyContribution(WX, WY, P.MountainFollyScale * 0.8f, P.MountainFollyAmplitude * 0.8f, P.MountainFollyBias, t, FS + 1);
		float C3 = ComputeFollyContribution(WX, WY, P.MountainFollyScale * 1.1f, P.MountainFollyAmplitude * 1.1f, P.MountainFollyBias, t, FS + 2);
		Height += FMath::Max3(C1, C2, C3);
		Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
	}

	bool bIsSea = (Height < P.SeaLevel);
	bool bIsMountain = (Height >= P.MountainStart);

	ComputeClimate(WX, WY, P, Result.Temperature, Result.Humidity);

	if (bIsSea)
	{
		float SeaFloor = TerrainNoise::Sample2D(
			S + (int32)ENoiseLayer::NSeaFloor * 7919,
			P.SeaFloorScale, 1, 0.5f, 2.0f, WX, WY) * P.SeaFloorAmplitude;
		float RawDepth = P.SeaLevel - Height;
		float EffectiveDepth = (RawDepth <= P.SeaMaxDepth) ? RawDepth : P.SeaMaxDepth + (RawDepth - P.SeaMaxDepth) * P.SeaDepthSlope;
		Height = P.SeaLevel - EffectiveDepth + SeaFloor;
		Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
		Result.WaterSurface = (uint16)FMath::Clamp(P.SeaLevel, 0.0f, (float)WORLD_HEIGHT - 1.0f);
		Result.Biome = EBiomeType::Plains;
		Result.bIsOcean = true;
	}

	if (bIsMountain)
	{
		Result.Biome = EBiomeType::Mountain;
		float MtnGrad = FMath::Clamp((Height - P.MountainStart) / P.BiomeBlendDistance, 0.0f, 1.0f);
		Height += TerrainNoise::Sample2D(
			S + (int32)ENoiseLayer::NMountainDetail * 7919,
			P.MountainDetailScale, 2, 0.5f, 2.0f, WX, WY) * P.MountainDetailAmplitude * MtnGrad;

		float MtnFactor = FMath::Clamp((Height - P.MountainStart) / (P.MaxHeight - P.MountainStart), 0.0f, 1.0f);
		float Lift = FMath::Sin(MtnFactor * PI * 0.5f);
		Height += TerrainNoise::Sample2D(
			S + (int32)ENoiseLayer::NMountainDetail * 7919 + 31337,
			P.MountainLiftScale, 1, 0.5f, 2.0f, WX, WY) * P.MountainLiftAmplitude * Lift;

		float RoughNoise = TerrainNoise::Sample2D(
			S + (int32)ENoiseLayer::NMountainRough * 7919,
			P.MountainRoughScale, 1, 0.5f, 2.0f, WX, WY);
		if (RoughNoise > P.MountainRoughThreshold)
		{
			float RoughMask = (RoughNoise - P.MountainRoughThreshold) / (1.0f - P.MountainRoughThreshold);
			float Detail1 = TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NMountainRough * 7919 + 7901,
				P.MountainRoughDetailScale, 1, 0.5f, 2.0f, WX, WY);
			float Detail2 = TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NMountainRough * 7919 + 7907,
				P.MountainRoughDetailScale * 2.0f, 1, 0.5f, 2.0f, WX, WY);
			Height += (Detail1 * 0.6f + Detail2 * 0.4f) * P.MountainRoughAmplitude * RoughMask * MtnFactor;
		}

		Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);
	}

	if (!bIsSea && !bIsMountain)
	{
		float EffGlacier = FMath::Lerp(P.GlacierThreshold, 1.0f, P.IceAgeFactor);
		EBiomeType Biome;
		if (Result.Temperature < EffGlacier) Biome = EBiomeType::Glacier;
		else
		{
			float FS = ScoreBiome(Result.Temperature, Result.Humidity, Height, P, P.ForestTempAffinity, P.ForestHumidAffinity, P.ForestHeightAffinity, P.ForestAdjust);
			float DS = ScoreBiome(Result.Temperature, Result.Humidity, Height, P, P.DesertTempAffinity, P.DesertHumidAffinity, P.DesertHeightAffinity, P.DesertAdjust);
			float PS = ScoreBiome(Result.Temperature, Result.Humidity, Height, P, P.PlainsTempAffinity, P.PlainsHumidAffinity, P.PlainsHeightAffinity, P.PlainsAdjust);
			if (FS >= DS && FS >= PS) Biome = EBiomeType::Forest;
			else if (DS >= PS) Biome = EBiomeType::Desert;
			else Biome = EBiomeType::Plains;
		}
		Result.Biome = Biome;

		float dSea = Height - P.SeaLevel, dMtn = P.MountainStart - Height, dGlac = FMath::Abs(Result.Temperature - EffGlacier);
		float Gradient = FMath::Clamp(FMath::Min3(dSea, dMtn, dGlac) / P.BiomeBlendDistance, 0.0f, 1.0f);

		if (Biome == EBiomeType::Plains)
			Height += FMath::Abs(TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NPlainsHill * 7919,
				P.PlainsHillScale, 1, 0.5f, 2.0f, WX, WY)) * P.PlainsHillAmplitude * Gradient;
		else if (Biome == EBiomeType::Desert)
			Height += FMath::Abs(TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NDesertDune * 7919,
				P.DesertDuneScale, 1, 0.5f, 2.0f, WX, WY)) * P.DesertDuneAmplitude * Gradient;
	}

	Height += TerrainNoise::Sample2D(
		S + (int32)ENoiseLayer::NMeso * 7919,
		P.MesoScale, P.MesoOctaves, P.MesoPersistence, P.MesoLacunarity,
		WX, WY) * P.MesoAmplitude;
	Height += TerrainNoise::Sample2D(
		S + (int32)ENoiseLayer::NMicro * 7919,
		P.MicroScale, P.MicroOctaves, P.MicroPersistence, P.MicroLacunarity,
		WX, WY) * P.MicroAmplitude;

	Height = FMath::Clamp(Height, 1.0f, P.MaxHeight);

	Result.Height = Height;
	return Result;
}

FColumnResult UWorldGeneratorComponent::ComputeColumnAt(int32 WX, int32 WY, const FGeneratorParams& P)
{
	return ComputeRawColumnAt(WX, WY, P);
}

void UWorldGeneratorComponent::GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P)
{
	const FIntPoint& CP = ChunkData.Position;
	const int32 SX = CP.X * CHUNK_SIZE, SY = CP.Y * CHUNK_SIZE, S = P.Seed;

	FColumnResult Results[CHUNK_SIZE][CHUNK_SIZE];
	for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
		for (int32 X = 0; X < CHUNK_SIZE; X++)
			Results[X][Y] = ComputeRawColumnAt(SX + X, SY + Y, P);

	bool bIsBeach[CHUNK_SIZE][CHUNK_SIZE];
	FMemory::Memzero(bIsBeach, sizeof(bIsBeach));

	for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
		for (int32 X = 0; X < CHUNK_SIZE; X++)
		{
			if (Results[X][Y].WaterSurface > 0) continue;
			uint16 MyH = (uint16)Results[X][Y].Height;
			const int32 DX[] = { -1, 1, 0, 0, -1, -1, 1, 1 }, DY[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
			for (int32 d = 0; d < 8; d++)
			{
				int32 NX = X + DX[d], NY = Y + DY[d];
				if (NX < 0 || NX >= CHUNK_SIZE || NY < 0 || NY >= CHUNK_SIZE) continue;
				if (Results[NX][NY].WaterSurface > 0 && MyH >= (uint16)Results[NX][NY].Height)
				{ bIsBeach[X][Y] = true; break; }
			}
		}

	for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
		for (int32 X = 0; X < CHUNK_SIZE; X++)
		{
			const FColumnResult& R = Results[X][Y];
			float Height = R.Height;
			const FBiomeParams& BP = BiomeParams[(int32)R.Biome];
			EBlockId SurfaceBlock = BP.SurfaceBlock;
			EBlockId SubsurfaceBlock = BP.SubsurfaceBlock;
			int32 SubsurfaceDepth = BP.SubsurfaceDepth;

			if (R.Biome == EBiomeType::Mountain)
			{
				if (Height > BP.SnowHeight) SurfaceBlock = EBlockId::Snow;
				else if (Height > BP.RockHeight) SurfaceBlock = EBlockId::Stone;
			}

			float N1 = TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NPerturb1 * 7919,
				P.PerturbScale, 1, 0.5f, 2.0f, SX + X, SY + Y);
			float N2 = TerrainNoise::Sample2D(
				S + (int32)ENoiseLayer::NPerturb2 * 7919,
				P.PerturbScale, 1, 0.5f, 2.0f, SX + X, SY + Y);
			int32 Perturb1 = (N1 > 0.3f) ? 1 : (N1 < -0.3f) ? -1 : 0;
			int32 Perturb2 = (N2 > 0.3f) ? 1 : (N2 < -0.3f) ? -1 : 0;

			int32 StoneBoundary = FMath::Clamp((int32)(Height - SubsurfaceDepth) + Perturb1, 0, WORLD_HEIGHT - 1);
			int32 SubBoundary = FMath::Clamp((int32)(Height - 1) + Perturb2, StoneBoundary + 1, WORLD_HEIGHT);

			uint16 EH = (uint16)Height;
			uint8 Top[TOP_LAYERS];
			for (int32 Layer = 0; Layer < TOP_LAYERS; Layer++)
			{
				int32 Z = (int32)EH - 1 - Layer;
				if (Z < 0) Top[Layer] = (uint8)EBlockId::Stone;
				else if (Z < StoneBoundary) Top[Layer] = (uint8)EBlockId::Stone;
				else if (Z < SubBoundary) Top[Layer] = (uint8)SubsurfaceBlock;
				else Top[Layer] = (uint8)SurfaceBlock;
			}

			if (R.WaterSurface > 0)
			{
				int32 FloorLayers = FMath::Min(P.WaterFloorDepth, TOP_LAYERS);
				for (int32 Layer = 0; Layer < FloorLayers; Layer++) Top[Layer] = (uint8)EBlockId::Sand;
			}

			if (bIsBeach[X][Y]) Top[0] = (uint8)EBlockId::Sand;

			ChunkData.SetColumn(X, Y, EH, Top);
			ChunkData.SetWaterColumn(X, Y, R.WaterSurface);
		}

	ChunkData.bIsGenerated = true;
}

void UWorldGeneratorComponent::GenerateChunk(FChunkData& ChunkData) { GenerateChunkData(ChunkData, CaptureParams()); }
EBiomeType UWorldGeneratorComponent::GetBiomeAt(int32 WX, int32 WY, const FGeneratorParams& P) { float G; return SelectBiome(WX, WY, P, G); }
float UWorldGeneratorComponent::GetHeight(int32 WorldX, int32 WorldY) const { return ComputeRawColumnAt(WorldX, WorldY, CaptureParams()).Height; }
const FBiomeParams& UWorldGeneratorComponent::GetBiomeParams(EBiomeType Biome) { return BiomeParams[static_cast<int32>(Biome)]; }
