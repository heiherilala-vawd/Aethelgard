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
    Glacier,
    MAX UMETA(Hidden)
};

enum class ENoiseLayer : int32
{
    NMacro = 0,
    NBaseShape,
    NMountainFolly,
    NBiomeSep,
    NTemperature,
    NHumidity,
    NTempPerturb,
    NHumidPerturb,
    NMeso,
    NMicro,
    NPlainsHill,
    NDesertDune,
    NMountainDetail,
    NSeaFloor,
    NMountainRough,
    NLake,
    NRiver,
    NVoronoi,
    NShoreDeform,
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
    float RockHeight = 0.0f;
    float SnowHeight = 0.0f;
    float HillAmp = 0.0f;
    float HillScale = 0.0f;
    bool bHasHillRelief = false;
    bool bHasDuneRelief = false;
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

    float MountainFollyScale = GenDef::MountainFollyScale;
    float MountainFollyAmplitude = GenDef::MountainFollyAmplitude;
    float MountainFollyBias = GenDef::MountainFollyBias;

    float BiomeSepScale = GenDef::BiomeSepScale;
    int32 BiomeSepOctaves = GenDef::BiomeSepOctaves;

    float TempScale = GenDef::TempScale;
    float HumidScale = GenDef::HumidScale;
    float TempPerturbScale = GenDef::TempPerturbScale;
    float TempPerturbAmplitude = GenDef::TempPerturbAmplitude;
    float HumidPerturbScale = GenDef::HumidPerturbScale;
    float HumidPerturbAmplitude = GenDef::HumidPerturbAmplitude;
    float GlacierThreshold = GenDef::GlacierThreshold;

    float IceAgeFactor = GenDef::IceAgeFactor;

    float TempWeight = GenDef::TempWeight;
    float HumidWeight = GenDef::HumidWeight;
    float HeightWeight = GenDef::HeightWeight;
    float AffinitySharpness = GenDef::AffinitySharpness;

    float ForestTempAffinity = GenDef::ForestTempAffinity;
    float ForestHumidAffinity = GenDef::ForestHumidAffinity;
    float ForestHeightAffinity = GenDef::ForestHeightAffinity;
    float ForestAdjust = GenDef::ForestAdjust;

    float DesertTempAffinity = GenDef::DesertTempAffinity;
    float DesertHumidAffinity = GenDef::DesertHumidAffinity;
    float DesertHeightAffinity = GenDef::DesertHeightAffinity;
    float DesertAdjust = GenDef::DesertAdjust;

    float PlainsTempAffinity = GenDef::PlainsTempAffinity;
    float PlainsHumidAffinity = GenDef::PlainsHumidAffinity;
    float PlainsHeightAffinity = GenDef::PlainsHeightAffinity;
    float PlainsAdjust = GenDef::PlainsAdjust;

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

    float PlainsHillScale = GenDef::PlainsHillScale;
    float PlainsHillAmplitude = GenDef::PlainsHillAmplitude;

    float DesertDuneScale = GenDef::DesertDuneScale;
    float DesertDuneAmplitude = GenDef::DesertDuneAmplitude;

    float MountainDetailScale = GenDef::MountainDetailScale;
    float MountainDetailAmplitude = GenDef::MountainDetailAmplitude;

    float MountainLiftScale = GenDef::MountainLiftScale;
    float MountainLiftAmplitude = GenDef::MountainLiftAmplitude;
    float MountainRoughScale = GenDef::MountainRoughScale;
    float MountainRoughThreshold = GenDef::MountainRoughThreshold;
    float MountainRoughAmplitude = GenDef::MountainRoughAmplitude;
    float MountainRoughDetailScale = GenDef::MountainRoughDetailScale;

    float MountainRockThreshold = GenDef::MountainRockThreshold;
    float MountainSnowThreshold = GenDef::MountainSnowThreshold;

    float BiomeBlendDistance = GenDef::BiomeBlendDistance;

    float LakeNoiseThreshold = GenDef::LakeNoiseThreshold;
    float RiverNoiseThreshold = GenDef::RiverNoiseThreshold;
    float LakeCircleDiameter = GenDef::LakeCircleDiameter;
    float RiverCircleDiameter = GenDef::RiverCircleDiameter;
    int32 LakeDepth = GenDef::LakeDepth;
    float LakeDepthSlope = GenDef::LakeDepthSlope;
    float RiverDepth = GenDef::RiverDepth;

    int32 WaterFloorDepth = GenDef::WaterFloorDepth;
    float SeaDepthSlope = GenDef::SeaDepthSlope;
    float SeaMaxDepth = GenDef::SeaMaxDepth;

    float SeaFloorScale = GenDef::SeaFloorScale;
    float SeaFloorAmplitude = GenDef::SeaFloorAmplitude;

    float BeachWidth = GenDef::BeachWidth;

    float VoronoiScale = GenDef::VoronoiScale;
    float LakeProbability = GenDef::LakeProbability;
    float LakeMaxDepth = GenDef::LakeMaxDepth;
    float LakeDepthFalloff = GenDef::LakeDepthFalloff;
    float LakeMinDiameter = GenDef::LakeMinDiameter;
    float LakeMaxDiameter = GenDef::LakeMaxDiameter;

    float ShoreDeformScale = GenDef::ShoreDeformScale;
    float ShoreDeformAmplitude = GenDef::ShoreDeformAmplitude;

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

    UPROPERTY(EditAnywhere, Category = "Generation|Folly")
    float MountainFollyScale = GenDef::MountainFollyScale;
    UPROPERTY(EditAnywhere, Category = "Generation|Folly")
    float MountainFollyAmplitude = GenDef::MountainFollyAmplitude;
    UPROPERTY(EditAnywhere, Category = "Generation|Folly")
    float MountainFollyBias = GenDef::MountainFollyBias;

    UPROPERTY(EditAnywhere, Category = "Biome|Separation")
    float BiomeSepScale = GenDef::BiomeSepScale;
    UPROPERTY(EditAnywhere, Category = "Biome|Separation")
    int32 BiomeSepOctaves = GenDef::BiomeSepOctaves;

    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float TempScale = GenDef::TempScale;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float HumidScale = GenDef::HumidScale;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float TempPerturbScale = GenDef::TempPerturbScale;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float TempPerturbAmplitude = GenDef::TempPerturbAmplitude;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float HumidPerturbScale = GenDef::HumidPerturbScale;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float HumidPerturbAmplitude = GenDef::HumidPerturbAmplitude;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float GlacierThreshold = GenDef::GlacierThreshold;
    UPROPERTY(EditAnywhere, Category = "Biome|Climate")
    float IceAgeFactor = GenDef::IceAgeFactor;

    UPROPERTY(EditAnywhere, Category = "Biome|Scoring")
    float TempWeight = GenDef::TempWeight;
    UPROPERTY(EditAnywhere, Category = "Biome|Scoring")
    float HumidWeight = GenDef::HumidWeight;
    UPROPERTY(EditAnywhere, Category = "Biome|Scoring")
    float HeightWeight = GenDef::HeightWeight;
    UPROPERTY(EditAnywhere, Category = "Biome|Scoring")
    float AffinitySharpness = GenDef::AffinitySharpness;

    UPROPERTY(EditAnywhere, Category = "Biome|Forest")
    float ForestTempAffinity = GenDef::ForestTempAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Forest")
    float ForestHumidAffinity = GenDef::ForestHumidAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Forest")
    float ForestHeightAffinity = GenDef::ForestHeightAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Forest")
    float ForestAdjust = GenDef::ForestAdjust;

    UPROPERTY(EditAnywhere, Category = "Biome|Desert")
    float DesertTempAffinity = GenDef::DesertTempAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Desert")
    float DesertHumidAffinity = GenDef::DesertHumidAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Desert")
    float DesertHeightAffinity = GenDef::DesertHeightAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Desert")
    float DesertAdjust = GenDef::DesertAdjust;

    UPROPERTY(EditAnywhere, Category = "Biome|Plains")
    float PlainsTempAffinity = GenDef::PlainsTempAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Plains")
    float PlainsHumidAffinity = GenDef::PlainsHumidAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Plains")
    float PlainsHeightAffinity = GenDef::PlainsHeightAffinity;
    UPROPERTY(EditAnywhere, Category = "Biome|Plains")
    float PlainsAdjust = GenDef::PlainsAdjust;

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

    UPROPERTY(EditAnywhere, Category = "Relief|Plains")
    float PlainsHillScale = GenDef::PlainsHillScale;
    UPROPERTY(EditAnywhere, Category = "Relief|Plains")
    float PlainsHillAmplitude = GenDef::PlainsHillAmplitude;

    UPROPERTY(EditAnywhere, Category = "Relief|Desert")
    float DesertDuneScale = GenDef::DesertDuneScale;
    UPROPERTY(EditAnywhere, Category = "Relief|Desert")
    float DesertDuneAmplitude = GenDef::DesertDuneAmplitude;

    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainDetailScale = GenDef::MountainDetailScale;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainDetailAmplitude = GenDef::MountainDetailAmplitude;

    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainLiftScale = GenDef::MountainLiftScale;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainLiftAmplitude = GenDef::MountainLiftAmplitude;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainRoughScale = GenDef::MountainRoughScale;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainRoughThreshold = GenDef::MountainRoughThreshold;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainRoughAmplitude = GenDef::MountainRoughAmplitude;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainRoughDetailScale = GenDef::MountainRoughDetailScale;

    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainRockThreshold = GenDef::MountainRockThreshold;
    UPROPERTY(EditAnywhere, Category = "Relief|Mountain")
    float MountainSnowThreshold = GenDef::MountainSnowThreshold;

    UPROPERTY(EditAnywhere, Category = "Biome")
    float BiomeBlendDistance = GenDef::BiomeBlendDistance;

    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    float LakeNoiseThreshold = GenDef::LakeNoiseThreshold;
    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    float LakeCircleDiameter = GenDef::LakeCircleDiameter;
    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    int32 LakeDepth = GenDef::LakeDepth;
    UPROPERTY(EditAnywhere, Category = "Water|Lake")
    float LakeDepthSlope = GenDef::LakeDepthSlope;

    UPROPERTY(EditAnywhere, Category = "Water|River")
    float RiverNoiseThreshold = GenDef::RiverNoiseThreshold;
    UPROPERTY(EditAnywhere, Category = "Water|River")
    float RiverCircleDiameter = GenDef::RiverCircleDiameter;
    UPROPERTY(EditAnywhere, Category = "Water|River")
    float RiverDepth = GenDef::RiverDepth;

    UPROPERTY(EditAnywhere, Category = "Water")
    int32 WaterFloorDepth = GenDef::WaterFloorDepth;
    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaDepthSlope = GenDef::SeaDepthSlope;
    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaMaxDepth = GenDef::SeaMaxDepth;

    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaFloorScale = GenDef::SeaFloorScale;
    UPROPERTY(EditAnywhere, Category = "Water")
    float SeaFloorAmplitude = GenDef::SeaFloorAmplitude;

    UPROPERTY(EditAnywhere, Category = "Water")
    float BeachWidth = GenDef::BeachWidth;

    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float VoronoiScale = GenDef::VoronoiScale;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float LakeProbability = GenDef::LakeProbability;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float LakeMaxDepth = GenDef::LakeMaxDepth;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float LakeDepthFalloff = GenDef::LakeDepthFalloff;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float LakeMinDiameter = GenDef::LakeMinDiameter;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Lake")
    float LakeMaxDiameter = GenDef::LakeMaxDiameter;

    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Shore")
    float ShoreDeformScale = GenDef::ShoreDeformScale;
    UPROPERTY(EditAnywhere, Category = "Water|Gen2|Shore")
    float ShoreDeformAmplitude = GenDef::ShoreDeformAmplitude;

    void GenerateChunk(FChunkData& ChunkData);
    float GetHeight(int32 WorldX, int32 WorldY) const;

    FGeneratorParams CaptureParams() const
    {
        FGeneratorParams P;
        P.Seed = Seed;
        P.MacroScale = MacroScale; P.MacroAmplitude = MacroAmplitude; P.MacroOctaves = MacroOctaves;
        P.MacroPersistence = MacroPersistence; P.MacroLacunarity = MacroLacunarity;
        P.BaseShapeScale = BaseShapeScale; P.BaseShapeAmplitude = BaseShapeAmplitude;
        P.BaseShapePersistence = BaseShapePersistence; P.BaseShapeLacunarity = BaseShapeLacunarity;
        P.MountainFollyScale = MountainFollyScale; P.MountainFollyAmplitude = MountainFollyAmplitude;
        P.MountainFollyBias = MountainFollyBias;
        P.BiomeSepScale = BiomeSepScale; P.BiomeSepOctaves = BiomeSepOctaves;
        P.TempScale = TempScale; P.HumidScale = HumidScale;         P.TempPerturbScale = TempPerturbScale; P.TempPerturbAmplitude = TempPerturbAmplitude;
        P.HumidPerturbScale = HumidPerturbScale; P.HumidPerturbAmplitude = HumidPerturbAmplitude;
        P.GlacierThreshold = GlacierThreshold;
        P.IceAgeFactor = IceAgeFactor;
        P.TempWeight = TempWeight; P.HumidWeight = HumidWeight; P.HeightWeight = HeightWeight; P.AffinitySharpness = AffinitySharpness;
        P.ForestTempAffinity = ForestTempAffinity; P.ForestHumidAffinity = ForestHumidAffinity; P.ForestHeightAffinity = ForestHeightAffinity; P.ForestAdjust = ForestAdjust;
        P.DesertTempAffinity = DesertTempAffinity; P.DesertHumidAffinity = DesertHumidAffinity; P.DesertHeightAffinity = DesertHeightAffinity; P.DesertAdjust = DesertAdjust;
        P.PlainsTempAffinity = PlainsTempAffinity; P.PlainsHumidAffinity = PlainsHumidAffinity; P.PlainsHeightAffinity = PlainsHeightAffinity; P.PlainsAdjust = PlainsAdjust;
        P.MesoScale = MesoScale; P.MesoAmplitude = MesoAmplitude; P.MesoOctaves = MesoOctaves;
        P.MesoPersistence = MesoPersistence; P.MesoLacunarity = MesoLacunarity;
        P.MicroScale = MicroScale; P.MicroAmplitude = MicroAmplitude; P.MicroOctaves = MicroOctaves;
        P.MicroPersistence = MicroPersistence; P.MicroLacunarity = MicroLacunarity;
        P.GlobalElevation = GlobalElevation;
        P.SeaLevel = SeaLevel; P.MountainStart = MountainStart; P.MaxHeight = MaxHeight;
        P.PlainsHillScale = PlainsHillScale; P.PlainsHillAmplitude = PlainsHillAmplitude;
        P.DesertDuneScale = DesertDuneScale; P.DesertDuneAmplitude = DesertDuneAmplitude;
        P.MountainDetailScale = MountainDetailScale; P.MountainDetailAmplitude = MountainDetailAmplitude;
        P.MountainLiftScale = MountainLiftScale; P.MountainLiftAmplitude = MountainLiftAmplitude;
        P.MountainRoughScale = MountainRoughScale; P.MountainRoughThreshold = MountainRoughThreshold;
        P.MountainRoughAmplitude = MountainRoughAmplitude; P.MountainRoughDetailScale = MountainRoughDetailScale;
        P.MountainRockThreshold = MountainRockThreshold; P.MountainSnowThreshold = MountainSnowThreshold;
        P.BiomeBlendDistance = BiomeBlendDistance;
        P.LakeNoiseThreshold = LakeNoiseThreshold; P.RiverNoiseThreshold = RiverNoiseThreshold;
        P.LakeCircleDiameter = LakeCircleDiameter; P.RiverCircleDiameter = RiverCircleDiameter;
        P.LakeDepth = LakeDepth; P.LakeDepthSlope = LakeDepthSlope;
        P.RiverDepth = RiverDepth;
        P.WaterFloorDepth = WaterFloorDepth;
        P.SeaDepthSlope = SeaDepthSlope; P.SeaMaxDepth = SeaMaxDepth;
        P.SeaFloorScale = SeaFloorScale; P.SeaFloorAmplitude = SeaFloorAmplitude;
        P.BeachWidth = BeachWidth;
        P.VoronoiScale = VoronoiScale;
        P.LakeProbability = LakeProbability;
        P.LakeMaxDepth = LakeMaxDepth;
        P.LakeDepthFalloff = LakeDepthFalloff;
        P.LakeMinDiameter = LakeMinDiameter;
        P.LakeMaxDiameter = LakeMaxDiameter;
        P.ShoreDeformScale = ShoreDeformScale;
        P.ShoreDeformAmplitude = ShoreDeformAmplitude;
        P.PerturbScale = GenDef::PerturbScale;
        return P;
    }

    static void GenerateChunkData(FChunkData& ChunkData, const FGeneratorParams& P);
    static EBiomeType GetBiomeAt(int32 WX, int32 WY, const FGeneratorParams& P);

private:
    static float ComputeBaseHeight(int32 WX, int32 WY, const FGeneratorParams& P);
    static EBiomeType SelectBiome(int32 WX, int32 WY, const FGeneratorParams& P, float& OutGradient);
    static FColumnResult ComputeColumnAt(int32 WX, int32 WY, const FGeneratorParams& P);
};
