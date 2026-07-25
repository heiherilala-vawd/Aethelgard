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
    Clay,
	Snow,
	Ice,
	LushGrass,
	MAX UMETA(Hidden)
};

struct FBlockDefinition
{
    FName Name;
    FName MaterialPath;
    float Hardness;
    float Resistance;
    bool bIsTransparent;
    bool bIsLiquid;
};

inline const FBlockDefinition& GetBlockDef(EBlockId Id)
{
    static const FBlockDefinition Definitions[] =
    {
        { NAME_None,   NAME_None, 0.0f, 0.0f, false, false },  // Air
        { TEXT("Stone"),   TEXT("/Game/Materials/Environment/M_Stone"),   1.5f, 6.0f, false, false },
        { TEXT("Dirt"),    TEXT("/Game/Materials/Environment/M_Dirt"),    0.5f, 0.5f, false, false },
        { TEXT("Grass"),   TEXT("/Game/Materials/Environment/M_Grass"),   0.6f, 0.6f, false, false },
        { TEXT("Sand"),    TEXT("/Game/Materials/Environment/M_Sand"),    0.5f, 0.5f, false, false },
        { TEXT("Water"),   TEXT("/Game/Materials/Liquid/M_Water"),        0.0f, 0.0f, true,  true  },
        { TEXT("Wood"),    TEXT("/Game/Materials/Environment/M_Wood"),    2.0f, 2.0f, false, false },
        { TEXT("Leaves"),  TEXT("/Game/Materials/Environment/M_Leaves"),  0.2f, 0.2f, true,  false },
        { TEXT("Clay"),    TEXT("/Game/Materials/Environment/M_Clay"),    0.5f, 1.0f, false, false },
        { TEXT("Snow"),    TEXT("/Game/Materials/Environment/M_Snow"),    0.3f, 0.3f, false, false },
	{ TEXT("Ice"),     TEXT("/Game/Materials/Environment/M_Ice"),     0.8f, 1.0f, true,  false },
	{ TEXT("LushGrass"),    TEXT("/Game/Materials/Environment/M_Grass"), 0.6f, 0.6f, false, false },
    };
    return Definitions[static_cast<int32>(Id)];
}

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
    case EBlockId::Clay:   return FColor(160, 82, 45);
    case EBlockId::Snow:   return FColor(240, 240, 240);
	case EBlockId::Ice:    return FColor(180, 210, 240);
	case EBlockId::LushGrass:    return FColor(20, 180, 20);
    default:               return FColor::Magenta;
    }
}
