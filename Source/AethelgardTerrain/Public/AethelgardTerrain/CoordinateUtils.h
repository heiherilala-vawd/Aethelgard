#pragma once

#include "CoreMinimal.h"
#include "AethelgardTerrain/ChunkData.h"

namespace TerrainCoords
{
	FORCEINLINE int32 FloorDiv(int32 A, int32 B)
	{
		int32 Q = A / B;
		int32 R = A % B;
		if (R != 0 && ((A < 0) != (B < 0))) Q--;
		return Q;
	}

	FORCEINLINE FIntPoint WorldToBlock(float WX, float WY, float BlockScale = 100.0f)
	{
		return FIntPoint(
			FMath::FloorToInt(WX / BlockScale),
			FMath::FloorToInt(WY / BlockScale)
		);
	}

	FORCEINLINE FIntPoint WorldToBlock(const FVector& WorldPos, float BlockScale = 100.0f)
	{
		return WorldToBlock(WorldPos.X, WorldPos.Y, BlockScale);
	}

	FORCEINLINE FIntPoint BlockToChunk(int32 BX, int32 BY, int32 ChunkSize = CHUNK_SIZE)
	{
		return FIntPoint(FloorDiv(BX, ChunkSize), FloorDiv(BY, ChunkSize));
	}

	FORCEINLINE int32 BlockToChunkLocal(int32 BlockCoord, int32 ChunkSize = CHUNK_SIZE)
	{
		int32 R = BlockCoord % ChunkSize;
		return R < 0 ? R + ChunkSize : R;
	}

	FORCEINLINE int32 ChunkToBlockOrigin(int32 ChunkCoord, int32 ChunkSize = CHUNK_SIZE)
	{
		return ChunkCoord * ChunkSize;
	}

	FORCEINLINE int32 BlockToRegionPixel(int32 BlockCoord, int32 PixelSize = 2)
	{
		return FloorDiv(BlockCoord, PixelSize);
	}

	constexpr int32 REGION_CHUNKS = 16;
	constexpr int32 REGION_BLOCKS = REGION_CHUNKS * CHUNK_SIZE;
	constexpr int32 HYDRO_PIXEL_SIZE = 2;
	constexpr int32 REGION_PIXELS = REGION_BLOCKS / HYDRO_PIXEL_SIZE;

	FORCEINLINE FIntPoint ChunkToRegion(FIntPoint Chunk, int32 RegionChunks = REGION_CHUNKS)
	{
		return FIntPoint(FloorDiv(Chunk.X, RegionChunks), FloorDiv(Chunk.Y, RegionChunks));
	}

	FORCEINLINE FIntPoint BlockToRegionPixel(int32 BX, int32 BY, int32 PixelSize = HYDRO_PIXEL_SIZE)
	{
		return FIntPoint(FloorDiv(BX, PixelSize), FloorDiv(BY, PixelSize));
	}

	FORCEINLINE int32 RegionPixelToBlock(int32 PixelCoord, int32 PixelSize = HYDRO_PIXEL_SIZE)
	{
		return PixelCoord * PixelSize;
	}
}
