// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlockRegistry.generated.h"

UENUM()
enum class EBlockId : uint8
{
    Air = 0,
    Stone,
    Dirt,
    Grass,
    Sand,
    Water,
    Wood,
    Leaves,
    MAX UMETA(Hidden)
};

struct FBlockDefinition
{
    FName Name;
    float Hardness;
    float Resistance;
    bool bIsTransparent;
    bool bIsLiquid;
};

inline FColor GetBlockColor(EBlockId BlockId, int32 FaceAxis = 2, int32 FaceDirection = 1)
{
    switch (BlockId)
    {
    case EBlockId::Stone:  return FColor(128, 128, 128);
    case EBlockId::Dirt:   return FColor(139, 90, 43);
    case EBlockId::Grass:
        return (FaceAxis == 2 && FaceDirection > 0)
            ? FColor(34, 139, 34)
            : FColor(139, 90, 43);
    case EBlockId::Sand:   return FColor(194, 178, 128);
    case EBlockId::Water:  return FColor(30, 100, 180);
    case EBlockId::Wood:   return FColor(101, 67, 33);
    case EBlockId::Leaves: return FColor(34, 120, 34);
    default:               return FColor::Magenta;
    }
}
