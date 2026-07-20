// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/BlockRegistry.h"
#include "ChunkData.generated.h"

constexpr int32 CHUNK_SIZE = 32;
constexpr int32 CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr int32 WORLD_HEIGHT = 512;
constexpr int32 TOP_LAYERS = 8;

USTRUCT()
struct AETHELGARDTERRAIN_API FChunkData
{
    GENERATED_BODY()

    FIntPoint Position;

    UPROPERTY()
    TArray<uint8> HeightData;

    UPROPERTY()
    TArray<uint8> TopBlocks;

    UPROPERTY()
    TMap<int32, uint8> Overrides;

    UPROPERTY()
    TArray<uint8> WaterLevel;

    bool bIsGenerated = false;
    bool bHasOverrides = false;

    void Initialize(const FIntPoint& InPosition)
    {
        Position = InPosition;
        HeightData.Init(0, CHUNK_AREA);
        TopBlocks.Init(static_cast<uint8>(EBlockId::Stone), CHUNK_AREA * TOP_LAYERS);
        WaterLevel.Init(0, CHUNK_AREA);
        Overrides.Empty();
        bIsGenerated = false;
        bHasOverrides = false;
    }

    static int32 GetColumnIndex(int32 X, int32 Y)
    {
        return Y * CHUNK_SIZE + X;
    }

    EBlockId GetBlock(int32 X, int32 Y, int32 Z) const
    {
        if (X < 0 || X >= CHUNK_SIZE || Y < 0 || Y >= CHUNK_SIZE || Z < 0 || Z >= WORLD_HEIGHT)
            return EBlockId::Air;

        int32 ColIdx = GetColumnIndex(X, Y);

        if (bHasOverrides)
        {
            int32 OvKey = Z * CHUNK_AREA + ColIdx;
            if (const uint8* Ov = Overrides.Find(OvKey))
                return static_cast<EBlockId>(*Ov);
        }

        uint8 H = HeightData[ColIdx];

        if (Z >= H)
        {
            if (Z < WaterLevel[ColIdx])
                return EBlockId::Water;
            return EBlockId::Air;
        }

        int32 LayerIdx = H - 1 - Z;
        if (LayerIdx < TOP_LAYERS)
            return static_cast<EBlockId>(TopBlocks[ColIdx * TOP_LAYERS + LayerIdx]);

        return EBlockId::Stone;
    }

    void SetBlock(int32 X, int32 Y, int32 Z, EBlockId Block)
    {
        if (X < 0 || X >= CHUNK_SIZE || Y < 0 || Y >= CHUNK_SIZE || Z < 0 || Z >= WORLD_HEIGHT)
            return;

        int32 ColIdx = GetColumnIndex(X, Y);
        int32 OvKey = Z * CHUNK_AREA + ColIdx;

        if (Block == EBlockId::Air)
            Overrides.Remove(OvKey);
        else
        {
            Overrides.Add(OvKey, static_cast<uint8>(Block));
            bHasOverrides = true;
        }
    }

    void SetColumn(int32 X, int32 Y, uint8 Height, const uint8 Top[TOP_LAYERS])
    {
        int32 ColIdx = GetColumnIndex(X, Y);
        HeightData[ColIdx] = Height;
        for (int32 i = 0; i < TOP_LAYERS; i++)
            TopBlocks[ColIdx * TOP_LAYERS + i] = Top[i];
    }

    void SetWaterColumn(int32 X, int32 Y, uint8 InWaterLevel)
    {
        WaterLevel[GetColumnIndex(X, Y)] = InWaterLevel;
    }

    uint8 GetWaterColumn(int32 X, int32 Y) const
    {
        return WaterLevel[GetColumnIndex(X, Y)];
    }
};
