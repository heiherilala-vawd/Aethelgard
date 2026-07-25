// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace GenDef
{
    constexpr float MacroScale = 0.0005f;
    constexpr float MacroAmplitude = 220.0f;
    constexpr int32 MacroOctaves = 2;
    constexpr float MacroPersistence = 0.5f;
    constexpr float MacroLacunarity = 4.0f;

    constexpr float BaseShapeScale = 0.001f;
    constexpr float BaseShapeAmplitude = 50.0f;
    constexpr float BaseShapePersistence = 0.5f;
    constexpr float BaseShapeLacunarity = 2.0f;

    constexpr float MountainFollyScale = 0.002f;
    constexpr float MountainFollyAmplitude = 280.0f;
    constexpr float MountainFollyBias = 0.55f;

    constexpr float BiomeSepScale = 0.04f;
    constexpr int32 BiomeSepOctaves = 2;

    constexpr float TempScale = 0.00015f;
    constexpr float HumidScale = 0.0004f;
    constexpr float TempPerturbScale = 0.001f;
    constexpr float TempPerturbAmplitude = 0.15f;
    constexpr float HumidPerturbScale = 0.001f;
    constexpr float HumidPerturbAmplitude = 0.15f;
    constexpr float GlacierThreshold = 0.30f;

    constexpr float MesoScale = 0.01f;
    constexpr float MesoAmplitude = 12.0f;
    constexpr int32 MesoOctaves = 2;
    constexpr float MesoPersistence = 0.5f;
    constexpr float MesoLacunarity = 2.0f;

    constexpr float MicroScale = 0.08f;
    constexpr float MicroAmplitude = 1.0f;
    constexpr int32 MicroOctaves = 1;
    constexpr float MicroPersistence = 0.5f;
    constexpr float MicroLacunarity = 2.0f;

    constexpr float GlobalElevation = 85.0f;

    constexpr float SeaLevel = 80.0f;
    constexpr float MountainStart = 150.0f;
    constexpr float MaxHeight = 260.0f;

    constexpr float PlainsHillScale = 0.03f;
    constexpr float PlainsHillAmplitude = 8.0f;

    constexpr float DesertDuneScale = 0.015f;
    constexpr float DesertDuneAmplitude = 10.0f;

    constexpr float MountainDetailScale = 0.01f;
    constexpr float MountainDetailAmplitude = 12.0f;

    constexpr float MountainLiftScale = 0.004f;
    constexpr float MountainLiftAmplitude = 40.0f;
    constexpr float MountainRoughScale = 0.015f;
    constexpr float MountainRoughThreshold = 0.35f;
    constexpr float MountainRoughAmplitude = 20.0f;
    constexpr float MountainRoughDetailScale = 0.008f;

    constexpr float MountainRockThreshold = 152.0f;
    constexpr float MountainSnowThreshold = 170.0f;

    constexpr float BiomeBlendDistance = 25.0f;

    constexpr float LakeNoiseThreshold = 0.65f;
    constexpr float RiverNoiseThreshold = 0.05f;
    constexpr float LakeCircleDiameter = 44.0f;
    constexpr float RiverCircleDiameter = 760.0f;
    constexpr int32 LakeDepth = 6;
    constexpr float LakeDepthSlope = 0.05f;
    constexpr float RiverDepth = 10.0f;

    constexpr int32 WaterFloorDepth = 3;
    constexpr float SeaDepthSlope = 0.03f;
    constexpr float SeaMaxDepth = 40.0f;

    constexpr float SeaFloorScale = 0.02f;
    constexpr float SeaFloorAmplitude = 5.0f;

    constexpr float BeachWidth = 15.0f;

    constexpr float CoastAmplitude = 3.0f;
    constexpr float SeaAttenuation = 3.0f;
    constexpr float LandAttenuation = 10.0f;
    constexpr float CoastalBlendSea = 10.0f;
    constexpr float CoastalBlendLand = 7.0f;

    constexpr float IceAgeFactor = 0.1f;

    constexpr float VoronoiScale = 0.001f;
    constexpr float LakeProbability = 0.5f;
    constexpr float LakeMaxDepth = 10.0f;
    constexpr float LakeDepthFalloff = 2.0f;
    constexpr float LakeMinDiameter = 100.0f;
    constexpr float LakeMaxDiameter = 5000.0f;

    constexpr float ShoreDeformScale = 0.05f;
    constexpr float ShoreDeformAmplitude = 3.0f;

    constexpr float TempWeight = 1.0f;
    constexpr float HumidWeight = 1.0f;
    constexpr float HeightWeight = 1.0f;
    constexpr float AffinitySharpness = 3.0f;

    constexpr float ForestTempAffinity = 0.75f;
    constexpr float ForestHumidAffinity = 0.8f;
    constexpr float ForestHeightAffinity = 0.45f;
    constexpr float ForestAdjust = 1.0f;

    constexpr float DesertTempAffinity = 0.9f;
    constexpr float DesertHumidAffinity = 0.15f;
    constexpr float DesertHeightAffinity = 0.40f;
    constexpr float DesertAdjust = 1.0f;

    constexpr float PlainsTempAffinity = 0.5f;
    constexpr float PlainsHumidAffinity = 0.5f;
    constexpr float PlainsHeightAffinity = 0.40f;
    constexpr float PlainsAdjust = 0.8f;

    constexpr float PerturbScale = 0.01f;

	constexpr float IceMtnTempAffinity = 0.2f;
	constexpr float IceMtnHumidAffinity = 0.5f;
	constexpr float IceMtnHeightAffinity = 0.65f;
	constexpr float IceMtnAdjust = 1.1f;

	constexpr float HumidMtnTempAffinity = 0.5f;
	constexpr float HumidMtnHumidAffinity = 0.9f;
	constexpr float HumidMtnHeightAffinity = 0.75f;
	constexpr float HumidMtnAdjust = 1.0f;

	constexpr float ClassicMtnTempAffinity = 0.5f;
	constexpr float ClassicMtnHumidAffinity = 0.5f;
	constexpr float ClassicMtnHeightAffinity = 0.65f;
	constexpr float ClassicMtnAdjust = 1.0f;

	constexpr float IceMtnPeakAmplitude = 55.0f;
	constexpr float ClassicMtnLiftAmplitude = 30.0f;
	constexpr float HumidMtnHillAmplitude = 10.0f;
	constexpr float HumidMtnHillScale = 0.008f;
}
