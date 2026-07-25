#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AethelgardTerrain/ChunkData.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "AethelgardTerrain/CoordinateUtils.h"
#include "AethelgardTerrain/RegionGenerator.h"
#include "ChunkManagerComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkReady, const FIntPoint&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChunkRemoved, const FIntPoint&);

UENUM()
enum class ERegionState : uint8
{
	Missing,
	Queued,
	Generating,
	Ready,
	Failed
};

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class AETHELGARDTERRAIN_API UChunkManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChunkManagerComponent();

	void SetWorldGenerator(UWorldGeneratorComponent* G) { Generator = G; }

	UPROPERTY(EditAnywhere, Category = "Chunks")
	int32 ViewDistance = 4;

	void UpdateCenter(const FIntPoint& CenterBlock);

	TSharedPtr<FChunkData> GetChunk(const FIntPoint& C) const;
	int32 GetChunkCount() const { return AllChunks.Num(); }
	int32 GetGenQueueNum() const { return ActiveRegionTasks; }
	int32 GetMeshQueueNum() const { return MeshQueue.Num(); }
	int32 GetActiveRegionTasks() const { return ActiveRegionTasks; }
	int32 GetRegionCount() const { return RegionCache.Num(); }

	EBlockId GetBlock(int32 BX, int32 BY, int32 BZ) const;
	bool SetBlock(int32 BX, int32 BY, int32 BZ, EBlockId Block);

	virtual void TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F) override;

	void InvalidateAll();

	FOnChunkReady OnChunkReadyForMesh;
	FOnChunkRemoved OnChunkRemoved;

private:
	UPROPERTY()
	UWorldGeneratorComponent* Generator = nullptr;

	int32 GenerationEpoch = 0;

	TMap<FIntPoint, TSharedPtr<FChunkData>> AllChunks;

	struct FRegionEntry
	{
		ERegionState State = ERegionState::Missing;
		int32 RequestToken = 0;
		FRegionGenerationOutput Data;
		TArray<FIntPoint> PendingChunks;
	};

	TMap<FIntPoint, FRegionEntry> RegionCache;

	TArray<FIntPoint> ChunkNeedsRegion;
	TArray<FIntPoint> MeshQueue;

	int32 ActiveRegionTasks = 0;
	static constexpr int32 MaxActiveRegions = 3;
	int32 NextRequestToken = 1;

	void EnsureRegion(const FIntPoint& RegionCoord);
	void LaunchRegionGen(const FIntPoint& RegionCoord);
	void OnRegionComplete(FIntPoint RegionCoord, int32 Token, FRegionGenerationOutput Data);
	void SliceRegionIntoChunks(const FIntPoint& RegionCoord);

	void EnqueueChunkNeedsRegion(const FIntPoint& C);
	void EnqueueMesh(const FIntPoint& C);
	void TryMesh(FIntPoint C);
	bool AllNeighborsReady(FIntPoint C) const;
	void PublishChunk(const FIntPoint& C, TSharedPtr<FChunkData> D);

	void DesiredCoords(const FIntPoint& Center, int32 R, TArray<FIntPoint>& Out) const;

	FIntPoint BlockToChunk(int32 BX, int32 BY) const
	{
		return TerrainCoords::BlockToChunk(BX, BY);
	}
};
