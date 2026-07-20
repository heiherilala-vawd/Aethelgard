// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AethelgardTerrain/ChunkData.h"
#include "AethelgardTerrain/GenerationDefaults.h"
#include "WorldGeneratorComponent.generated.h"

UENUM()
enum class EBiomeType : uint8
{
    Plains = 0,
    Desert,
    Mountain,
    Forest,
    MAX UMETA(Hidden)
};

enum class ENoiseLayer : int32
{
    NMacro = 0,
    NBaseShape,
    NMeso,
    NMicro,
    NVoronoi,
    NMountainShape,
    NHills,
    NLake,
    NPerturb1,
    NPerturb2,
    NMAX UMETA(Hidden)
};

USTRUCT()
struct AETHELGARDTERRAIN_API FBiomeParams
{
    GENERATED_BODY()

    EBlockId SurfaceBlock = EBlockId::Grass;
    EBlockId SubsurfaceBlock = EBlockId::Dirt;
    int32 SubsurfaceDepth = 3;
    bool bHasHills = false;
    float HillAmplitude = 0.0f;
    float MinHeight = 70.0f;
    float MaxHeight = 110.0f;
};

USTRUCT()
struct AETHELGARDTERRAIN_API FGeneratorParams
{
    GENERATED_BODY()

    int32 Seed = 0;

    float MacroScale = GenDef::MacroScale;
    float MacroAmplitude = GenDef::MacroAmplitude;
    int32 MacroOctaves = GenDef::MacroOctaves;
    float MacroPersistence = GenDef::MacroPersistence;
    float MacroLacunarity = GenDef::MacroLacunarity;

    float BaseShapeScale = GenDef::BaseShapeScale;
    float BaseShapeAmplitude = GenDef::BaseShapeAmplitude;
    float BaseShapePersistence = GenDef::BaseShapePersistence;
    float BaseShapeLacunarity = GenDef::BaseShapeLacunarity;

    float MesoScale = GenDef::MesoScale;
    float MesoAmplitude = GenDef::MesoAmplitude;
    int32 MesoOctaves = GenDef::MesoOctaves;
    float MesoPersistence = GenDef::MesoPersistence;
    float MesoLacunarity = GenDef::MesoLacunarity;

    float MicroScale = GenDef::MicroScale;
    float MicroAmplitude = GenDef::MicroAmplitude;
    int32 MicroOctaves = GenDef::MicroOctaves;
    float MicroPersistence = GenDef::MicroPersistence;
    float MicroLacunarity = GenDef::MicroLacunarity;

    float GlobalElevation = GenDef::GlobalElevation;

    float SeaLevel = GenDef::SeaLevel;
    float MountainStart = GenDef::MountainStart;
    float MaxHeight = GenDef::MaxHeight;

    float VoronoiScale = GenDef::VoronoiScale;

    float LakeThreshold = GenDef::LakeThreshold;
    int32 LakeDepth = GenDef::LakeDepth;
    int32 WaterFloorDepth = GenDef::WaterFloorDepth;
    float SeaDepthSlope = GenDef::SeaDepthSlope;
    float SeaMaxDepth = GenDef::SeaMaxDepth;
    float LakeDepthSlope = GenDef::LakeDepthSlope;

    float MountainShapeScale = GenDef::MountainShapeScale;
    float MountainShapeAmplitude = GenDef::MountainShapeAmplitude;
    float MountainShapePersistence = GenDef::MountainShapePersistence;
    float MountainShapeLacunarity = GenDef::MountainShapeLacunarity;

    float HillScale = GenDef::HillScale;
    float HillAmplitude = GenDef::HillAmplitude;

    float PerturbScale = GenDef::PerturbScale;
};

struct AETHELGARDTERRAIN_API FColumnResult
{
    float Height = 0.0f;
    EBiomeType Biome = EBiomeType::Plains;
    uint8 WaterSurface = 0;
};

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class AETHELGARDTERRAIN_API UWorldGeneratorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "World")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroScale = GenDef::MacroScale;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroAmplitude = GenDef::MacroAmplitude;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    int32 MacroOctaves = GenDef::MacroOctaves;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroPersistence = GenDef::MacroPersistence;

    UPROPERTY(EditAnywhere, Category = "Generation|Macro")
    float MacroLacunarity = GenDef::MacroLacunarity;

    UPROPERTY(EditAnywhere, Category = "Generation|BaseShape")
    float BaseShapeScale = GenDef::BaseShapeScale;

    UPROPERTY(EditAnywhere, Category = "Generation|BaseShape")
    float BaseShapeAmplitude = GenDef::BaseShapeAmplitude;

    UPROPERTY(EditAnywhere, Category = "Generation|BaseShape")
    float BaseShapePersistence = GenDef::BaseShapePersistence;

    UPROPERTY(EditAnywhere, Category = "Generation|BaseShape")
    float BaseShapeLacunarity = GenDef::BaseShapeLacunarity;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoScale = GenDef::MesoScale;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoAmplitude = GenDef::MesoAmplitude;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    int32 MesoOctaves = GenDef::MesoOctaves;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoPersistence = GenDef::MesoPersistence;

    UPROPERTY(EditAnywhere, Category = "Generation|Meso")
    float MesoLacunarity = GenDef::MesoLacunarity;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroScale = GenDef::MicroScale;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroAmplitude = GenDef::MicroAmplitude;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    int32 MicroOctaves = GenDef::MicroOctaves;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroPersistence = GenDef::MicroPersistence;

    UPROPERTY(EditAnywhere, Category = "Generation|Micro")
    float MicroLacunarity = GenDef::MicroLacunarity;

    UPROPERTY(EditAnywhere, Category = "Generation|Global")
    float GlobalElevation = GenDef::GlobalElevation;

    UPROPERTY(EditAnywhere, Category = "Zoning")
    float SeaLevel = GenDef::SeaLevel;

    UPROPERTY(EditAnywhere, Category = "Zoning")
    float MountainStart = GenDef::MountainStart;

    UPROPERTY(EditAnywhere, Category = "Zoning")
    float MaxHeight = GenDef::MaxHeight;

    UPROPERTY(EditAnywhere, Category = "Biome|Voronoi")
    float VoronoiScale = GenDef::VoronoiScale;

    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    float LakeThreshold = GenDef::LakeThreshold;

    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    int32 LakeDepth = GenDef::LakeDepth;

    UPROPERTY(EditAnywhere, Category = "Water")
    int32 WaterFloorDepth = GenDef::WaterFloorDepth;

    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaDepthSlope = GenDef::SeaDepthSlope;

    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaMaxDepth = GenDef::SeaMaxDepth;

    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    float LakeDepthSlope = GenDef::LakeDepthSlope;

    UPROPERTY(EditAnywhere, Category = "Mountain")
    float MountainShapeScale = GenDef::MountainShapeScale;

    UPROPERTY(EditAnywhere, Category = "Mountain")
    float MountainShapeAmplitude = GenDef::MountainShapeAmplitude;

    UPROPERTY(EditAnywhere, Category = "Mountain")
    float MountainShapePersistence = GenDef::MountainShapePersistence;

    UPROPERTY(EditAnywhere, Category = "Mountain")
    float MountainShapeLacunarity = GenDef::MountainShapeLacunarity;

    UPROPERTY(EditAnywhere, Category = "Biome|Hills")
    float HillScale = GenDef::HillScale;

    UPROPERTY(EditAnywhere, Category = "Biome|Hills")
    float HillAmplitude = GenDef::HillAmplitude;

    void GenerateChunk(FChunkData& ChunkData);
    float GetHeight(int32 WorldX, int32 WorldY) const;

    FGeneratorParams CaptureParams() const
    {
        FGeneratorParams P;
        P.Seed = Seed;
        P.MacroScale = MacroScale;
        P.MacroAmplitude = MacroAmplitude;
        P.MacroOctaves = MacroOctaves;
        P.MacroPersistence = MacroPersistence;
        P.MacroLacunarity = MacroLacunarity;
        P.BaseShapeScale = BaseShapeScale;
        P.BaseShapeAmplitude = BaseShapeAmplitude;
        P.BaseShapePersistence = BaseShapePersistence;
        P.BaseShapeLacunarity = BaseShapeLacunarity;
        P.MesoScale = MesoScale;
        P.MesoAmplitude = MesoAmplitude;
        P.MesoOctaves = MesoOctaves;
        P.MesoPersistence = MesoPersistence;
        P.MesoLacunarity = MesoLacunarity;
        P.MicroScale = MicroScale;
        P.MicroAmplitude = MicroAmplitude;
        P.MicroOctaves = MicroOctaves;
        P.MicroPersistence = MicroPersistence;
        P.MicroLacunarity = MicroLacunarity;
        P.GlobalElevation = GlobalElevation;
        P.SeaLevel = SeaLevel;
        P.MountainStart = MountainStart;
        P.MaxHeight = MaxHeight;
        P.VoronoiScale = VoronoiScale;
        P.LakeThreshold = LakeThreshold;
        P.LakeDepth = LakeDepth;
        P.WaterFloorDepth = WaterFloorDepth;
        P.SeaDepthSlope = SeaDepthSlope;
        P.SeaMaxDepth = SeaMaxDepth;
        P.LakeDepthSlope = LakeDepthSlope;
        P.MountainShapeScale = MountainShapeScale;
        P.MountainShapeAmplitude = MountainShapeAmplitude;
        P.MountainShapePersistence = MountainShapePersistence;
        P.MountainShapeLacunarity = MountainShapeLacunarity;
        P.HillScale = HillScale;
        P.HillAmplitude = HillAmplitude;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
    static EBiomeType GetBiomeAt(int32 WX, int32 WY, const FGeneratorParams& P);

private:
    static float ComputeBaseHeight(int32 WX, int32 WY, const FGeneratorParams& P);
    static int32 VoronoiSelect(int32 WX, int32 WY, float Scale, int32 Seed);
    static FColumnResult ComputeColumnAt(int32 WX, int32 WY, const FGeneratorParams& P);
};
