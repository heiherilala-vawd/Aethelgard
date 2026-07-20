// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace GenDef
{
    constexpr float MacroScale = 0.0002f;
    constexpr float MacroAmplitude = 130.0f;
    constexpr int32 MacroOctaves = 2;
    constexpr float MacroPersistence = 0.7f;
    constexpr float MacroLacunarity = 4.0f;

    constexpr float BaseShapeScale = 0.001f;
    constexpr float BaseShapeAmplitude = 15.0f;
    constexpr float BaseShapePersistence = 0.5f;
    constexpr float BaseShapeLacunarity = 2.0f;

    constexpr float MesoScale = 0.025f;
    constexpr float MesoAmplitude = 8.0f;
    constexpr int32 MesoOctaves = 2;
    constexpr float MesoPersistence = 0.5f;
    constexpr float MesoLacunarity = 2.0f;

    constexpr float MicroScale = 0.08f;
    constexpr float MicroAmplitude = 3.0f;
    constexpr int32 MicroOctaves = 1;
    constexpr float MicroPersistence = 0.5f;
    constexpr float MicroLacunarity = 2.0f;

    constexpr float GlobalElevation = 75.0f;

    constexpr float SeaLevel = 70.0f;
    constexpr float MountainStart = 110.0f;
    constexpr float MaxHeight = 200.0f;

    constexpr float VoronoiScale = 0.002f;

    constexpr float LakeThreshold = 95.0f;
    constexpr int32 LakeDepth = 6;
    constexpr int32 WaterFloorDepth = 3;
    constexpr float SeaDepthSlope = 0.05f;
    constexpr float SeaMaxDepth = 25.0f;
    constexpr float LakeDepthSlope = 0.05f;

    constexpr float MountainShapeScale = 0.03f;
    constexpr float MountainShapeAmplitude = 90.0f;
    constexpr float MountainShapePersistence = 0.5f;
    constexpr float MountainShapeLacunarity = 2.0f;

    constexpr float HillScale = 0.1f;
    constexpr float HillAmplitude = 2.0f;

    constexpr float PerturbScale = 0.01f;
    constexpr float LakeNoiseScale = 0.01f;

    constexpr float MountainStoneThreshold = 120.0f;

    constexpr float PlainsMinHeight = 1.0f;
    constexpr float PlainsMaxHeight = 110.0f;
    constexpr float DesertMinHeight = 1.0f;
    constexpr float DesertMaxHeight = 95.0f;
    constexpr float ForestMinHeight = 1.0f;
    constexpr float ForestMaxHeight = 110.0f;
}
