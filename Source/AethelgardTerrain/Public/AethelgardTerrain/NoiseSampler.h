#pragma once

#include "CoreMinimal.h"

namespace TerrainNoise
{
	FORCEINLINE float Sample2D(int32 Seed, float Scale, int32 Octaves, float Persistence, float Lacunarity,
		int32 WorldX, int32 WorldY)
	{
		float Value = 0.0f, Amplitude = 1.0f, MaxAmp = 0.0f, Freq = 1.0f;
		for (int32 i = 0; i < Octaves; i++)
		{
			int32 Key = Seed + i * 7919;
			FRandomStream Stream(Key);
			FVector2D Off(Stream.FRand() * 10000.0f, Stream.FRand() * 10000.0f);
			Value += Amplitude * FMath::PerlinNoise2D(FVector2D(
				(float)WorldX * Scale * Freq + Off.X,
				(float)WorldY * Scale * Freq + Off.Y));
			MaxAmp += Amplitude;
			Amplitude *= Persistence;
			Freq *= Lacunarity;
		}
		return Value / MaxAmp;
	}
}
