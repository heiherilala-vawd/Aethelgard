// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Terrain/BlockRegistry.h"
#include "ChunkData.generated.h"

constexpr int32 CHUNK_SIZE = 32;
constexpr int32 CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr int32 WORLD_HEIGHT = 256;

USTRUCT()
struct FChunkData
{
    GENERATED_BODY()

    FIntPoint Position;
    TArray<uint8> Blocks;
    bool bIsGenerated = false;

    void Initialize(const FIntPoint& InPosition)
    {
        Position = InPosition;
        Blocks.Init(static_cast<uint8>(EBlockId::Air), CHUNK_AREA * WORLD_HEIGHT);
        bIsGenerated = false;
    }

    int32 GetIndex(int32 X, int32 Y, int32 Z) const
    {
        return (Z * CHUNK_SIZE + Y) * CHUNK_SIZE + X;
    }

    EBlockId GetBlock(int32 X, int32 Y, int32 Z) const
    {
        if (X < 0 || X >= CHUNK_SIZE || Y < 0 || Y >= CHUNK_SIZE || Z < 0 || Z >= WORLD_HEIGHT)
            return EBlockId::Air;
        return static_cast<EBlockId>(Blocks[GetIndex(X, Y, Z)]);
    }

    void SetBlock(int32 X, int32 Y, int32 Z, EBlockId InBlock)
    {
        if (X < 0 || X >= CHUNK_SIZE || Y < 0 || Y >= CHUNK_SIZE || Z < 0 || Z >= WORLD_HEIGHT)
            return;
        Blocks[GetIndex(X, Y, Z)] = static_cast<uint8>(InBlock);
    }
};
