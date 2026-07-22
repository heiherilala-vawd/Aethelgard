#include "AethelgardTerrain/ChunkManagerComponent.h"
#include "AethelgardTerrain/GenerationDefaults.h"
#include "AethelgardTerrain/NoiseSampler.h"
#include "Async/Async.h"

UChunkManagerComponent::UChunkManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UChunkManagerComponent::InvalidateAll()
{
	GenerationEpoch++;
	NextRequestToken = FMath::Max(NextRequestToken, GenerationEpoch + 1);
	AllChunks.Empty();
	RegionCache.Empty();
	ChunkNeedsRegion.Empty();
	MeshQueue.Empty();
	ActiveRegionTasks = 0;
}

void UChunkManagerComponent::UpdateCenter(const FIntPoint& CenterBlock)
{
	int32 AppearanceRadius = ViewDistance;
	int32 RetentionRadius = ViewDistance * 2;

	TArray<FIntPoint> AppearanceSet;
	DesiredCoords(CenterBlock, AppearanceRadius + 1, AppearanceSet);

	TSet<FIntPoint> Appearance;
	for (const FIntPoint& C : AppearanceSet)
		Appearance.Add(C);

	TArray<FIntPoint> RetentionSet;
	DesiredCoords(CenterBlock, RetentionRadius + 1, RetentionSet);

	TSet<FIntPoint> Retain;
	for (const FIntPoint& C : RetentionSet)
		Retain.Add(C);

	TArray<FIntPoint> ToRemove;
	ToRemove.Reserve(64);
	for (const auto& KV : AllChunks)
		if (!Retain.Contains(KV.Key))
			ToRemove.Add(KV.Key);

	for (const FIntPoint& C : ToRemove)
	{
		AllChunks.Remove(C);
		OnChunkRemoved.Broadcast(C);
	}

	for (const FIntPoint& C : Appearance)
	{
		if (AllChunks.Contains(C)) continue;

		TSharedPtr<FChunkData> D = MakeShared<FChunkData>();
		D->Initialize(C);
		AllChunks.Add(C, D);

		FIntPoint RegionCoord = TerrainCoords::ChunkToRegion(C, TerrainRegion::REGION_CHUNKS);
		FRegionEntry* Entry = RegionCache.Find(RegionCoord);

		if (Entry && Entry->State == ERegionState::Ready)
		{
			SliceRegionIntoChunks(RegionCoord);
		}
		else
		{
			EnqueueChunkNeedsRegion(C);
			EnsureRegion(RegionCoord);
		}
	}

	TArray<FIntPoint> StaleRegions;
	for (auto& Pair : RegionCache)
	{
		bool bNeeded = false;
		for (int32 DY = 0; DY < TerrainRegion::REGION_CHUNKS; DY++)
			for (int32 DX = 0; DX < TerrainRegion::REGION_CHUNKS; DX++)
			{
				FIntPoint Chunk(Pair.Key.X * TerrainRegion::REGION_CHUNKS + DX,
					Pair.Key.Y * TerrainRegion::REGION_CHUNKS + DY);
				if (Retain.Contains(Chunk)) { bNeeded = true; break; }
			}
		if (!bNeeded && Pair.Value.State != ERegionState::Generating)
			StaleRegions.Add(Pair.Key);
	}

	for (const FIntPoint& R : StaleRegions)
		RegionCache.Remove(R);
}

void UChunkManagerComponent::EnsureRegion(const FIntPoint& RegionCoord)
{
	FRegionEntry& Entry = RegionCache.FindOrAdd(RegionCoord);
	if (Entry.State == ERegionState::Missing)
	{
		Entry.State = ERegionState::Queued;
	}
}

void UChunkManagerComponent::TickComponent(float DT, ELevelTick T, FActorComponentTickFunction* F)
{
	Super::TickComponent(DT, T, F);

	for (auto& Pair : RegionCache)
	{
		if (Pair.Value.State == ERegionState::Queued && ActiveRegionTasks < MaxActiveRegions)
			LaunchRegionGen(Pair.Key);
	}

	for (int32 i = 0; i < 3 && MeshQueue.Num() > 0; i++)
	{
		FIntPoint C = MeshQueue.Last();
		MeshQueue.RemoveAt(MeshQueue.Num() - 1);
		OnChunkReadyForMesh.Broadcast(C);
	}
}

void UChunkManagerComponent::LaunchRegionGen(const FIntPoint& RegionCoord)
{
	FRegionEntry* Entry = RegionCache.Find(RegionCoord);
	if (!Entry || Entry->State != ERegionState::Queued) return;

	Entry->State = ERegionState::Generating;
	Entry->RequestToken = NextRequestToken++;
	ActiveRegionTasks++;

	FRegionGenerationInput Input;
	Input.RegionCoord = RegionCoord;
	Input.Seed = Generator->Seed;
	Input.GenerationVersion = GenerationEpoch;
	Input.Params = Generator->CaptureParams();
	Input.HydroSettings = FHydrologySettings();

	int32 Token = Entry->RequestToken;
	TWeakObjectPtr<UChunkManagerComponent> WeakSelf(this);

	Async(EAsyncExecution::ThreadPool, [Input, RegionCoord, Token, WeakSelf]()
	{
		FRegionGenerationOutput Output = TerrainRegionGenerator::GenerateRegion(Input);

		AsyncTask(ENamedThreads::GameThread, [WeakSelf, RegionCoord, Token, Output = MoveTemp(Output)]()
		{
			if (!WeakSelf.IsValid()) return;
			WeakSelf->OnRegionComplete(RegionCoord, Token, Output);
		});
	});
}

void UChunkManagerComponent::OnRegionComplete(FIntPoint RegionCoord, int32 Token, FRegionGenerationOutput Data)
{
	ActiveRegionTasks--;

	FRegionEntry* Entry = RegionCache.Find(RegionCoord);
	if (!Entry) return;
	if (Entry->RequestToken != Token) return;

	Entry->State = ERegionState::Ready;
	Entry->Data = MoveTemp(Data);

	for (const FIntPoint& C : Entry->PendingChunks)
	{
		if (AllChunks.Find(C))
		{
			SliceRegionIntoChunks(RegionCoord);
			break;
		}
	}
}

void UChunkManagerComponent::SliceRegionIntoChunks(const FIntPoint& RegionCoord)
{
	FRegionEntry* Entry = RegionCache.Find(RegionCoord);
	if (!Entry || Entry->State != ERegionState::Ready) return;

	const FRegionGenerationOutput& Data = Entry->Data;
	const int32 CorePX = Data.PixelWidth;
	const int32 PixelSize = TerrainRegion::HYDRO_PIXEL_SIZE;

	TArray<FIntPoint> ChunksToFill;
	for (auto& Pair : AllChunks)
	{
		FIntPoint ChunkRegion = TerrainCoords::ChunkToRegion(Pair.Key, TerrainRegion::REGION_CHUNKS);
		if (ChunkRegion == RegionCoord && Pair.Value.IsValid() && !Pair.Value->bIsGenerated)
			ChunksToFill.Add(Pair.Key);
	}

	for (const FIntPoint& ChunkCoord : ChunksToFill)
	{
		TSharedPtr<FChunkData>* Found = AllChunks.Find(ChunkCoord);
		if (!Found || !Found->IsValid()) continue;
		FChunkData& CD = *(*Found);
		if (CD.bIsGenerated) continue;

		int32 LocalChunkX = ChunkCoord.X - RegionCoord.X * TerrainRegion::REGION_CHUNKS;
		int32 LocalChunkY = ChunkCoord.Y - RegionCoord.Y * TerrainRegion::REGION_CHUNKS;

		for (int32 BY = 0; BY < CHUNK_SIZE; BY++)
			for (int32 BX = 0; BX < CHUNK_SIZE; BX++)
			{
				int32 WorldBX = ChunkCoord.X * CHUNK_SIZE + BX;
				int32 WorldBY = ChunkCoord.Y * CHUNK_SIZE + BY;
				int32 PX = (WorldBX - Data.BlockOrigin.X) / PixelSize;
				int32 PY = (WorldBY - Data.BlockOrigin.Y) / PixelSize;

				float Height = 0.0f;
				float WaterSurf = 0.0f;
				EWaterType WType = EWaterType::None;

				if (PX >= 0 && PX < CorePX && PY >= 0 && PY < CorePX)
				{
					int32 PIdx = PY * CorePX + PX;
					Height = Data.FinalHeight[PIdx];
					WaterSurf = Data.WaterSurface[PIdx];
					WType = Data.WaterType[PIdx];
				}
				else
				{
					FColumnResult CR = UWorldGeneratorComponent::ComputeRawColumnAt(WorldBX, WorldBY,
						Generator->CaptureParams());
					Height = CR.Height;
					WaterSurf = (float)CR.WaterSurface;
				}

			uint16 EH = (uint16)FMath::Clamp(Height, 1.0f, (float)WORLD_HEIGHT - 1.0f);
			uint16 WS = (uint16)FMath::Clamp(WaterSurf, 0.0f, (float)WORLD_HEIGHT - 1.0f);

				uint8 Top[TOP_LAYERS];

				EBiomeType Biome = EBiomeType::Plains;
				if (PX >= 0 && PX < CorePX && PY >= 0 && PY < CorePX)
					Biome = Data.Biome[PY * CorePX + PX];

				const FBiomeParams& BP = UWorldGeneratorComponent::GetBiomeParams(Biome);
				EBlockId SurfaceBlock = BP.SurfaceBlock;
				EBlockId SubsurfaceBlock = BP.SubsurfaceBlock;
				int32 SubsurfaceDepth = BP.SubsurfaceDepth;

				if (Biome == EBiomeType::Mountain)
				{
					if (Height > BP.SnowHeight) SurfaceBlock = EBlockId::Snow;
					else if (Height > BP.RockHeight) SurfaceBlock = EBlockId::Stone;
				}

				int32 S = Generator ? Generator->Seed : 0;
				float N1 = TerrainNoise::Sample2D(
					S + (int32)ENoiseLayer::NPerturb1 * 7919,
					GenDef::PerturbScale, 1, 0.5f, 2.0f, WorldBX, WorldBY);
				float N2 = TerrainNoise::Sample2D(
					S + (int32)ENoiseLayer::NPerturb2 * 7919,
					GenDef::PerturbScale, 1, 0.5f, 2.0f, WorldBX, WorldBY);
				int32 Perturb1 = (N1 > 0.3f) ? 1 : (N1 < -0.3f) ? -1 : 0;
				int32 Perturb2 = (N2 > 0.3f) ? 1 : (N2 < -0.3f) ? -1 : 0;

				int32 StoneBoundary = FMath::Clamp((int32)(Height - SubsurfaceDepth) + Perturb1, 0, WORLD_HEIGHT - 1);
				int32 SubBoundary = FMath::Clamp((int32)(Height - 1) + Perturb2, StoneBoundary + 1, WORLD_HEIGHT);

				for (int32 L = 0; L < TOP_LAYERS; L++)
				{
					int32 Z = (int32)EH - 1 - L;
					if (Z < 0) Top[L] = (uint8)EBlockId::Stone;
					else if (Z < StoneBoundary) Top[L] = (uint8)EBlockId::Stone;
					else if (Z < SubBoundary) Top[L] = (uint8)SubsurfaceBlock;
					else Top[L] = (uint8)SurfaceBlock;
				}

				if (WS > 0)
				{
					int32 FloorLayers = FMath::Min(GenDef::WaterFloorDepth, TOP_LAYERS);
					for (int32 L = 0; L < FloorLayers; L++) Top[L] = (uint8)EBlockId::Sand;
				}

				if (WS == 0 && PX >= 0 && PX < CorePX && PY >= 0 && PY < CorePX)
				{
					const int32 DX[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
					const int32 DY[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
					for (int32 d = 0; d < 8; d++)
					{
						int32 NPX = PX + DX[d], NPY = PY + DY[d];
						if (NPX < 0 || NPX >= CorePX || NPY < 0 || NPY >= CorePX) continue;
						if (Data.WaterSurface[NPY * CorePX + NPX] > 0.0f && EH >= (uint16)Data.WaterSurface[NPY * CorePX + NPX])
						{
							Top[0] = (uint8)EBlockId::Sand;
							break;
						}
					}
				}

				CD.SetColumn(BX, BY, EH, Top);
				CD.SetWaterColumn(BX, BY, WS);
			}

		CD.bIsGenerated = true;

		TryMesh(ChunkCoord);

		for (int32 DY = -1; DY <= 1; DY++)
			for (int32 DX = -1; DX <= 1; DX++)
				if (DX != 0 || DY != 0)
					TryMesh(FIntPoint(ChunkCoord.X + DX, ChunkCoord.Y + DY));
	}
}

void UChunkManagerComponent::EnqueueChunkNeedsRegion(const FIntPoint& C)
{
	ChunkNeedsRegion.AddUnique(C);

	FIntPoint RC = TerrainCoords::ChunkToRegion(C, TerrainRegion::REGION_CHUNKS);
	FRegionEntry* Entry = RegionCache.Find(RC);
	if (Entry) Entry->PendingChunks.AddUnique(C);
}

void UChunkManagerComponent::EnqueueMesh(const FIntPoint& C)
{
	MeshQueue.AddUnique(C);
}

void UChunkManagerComponent::TryMesh(FIntPoint C)
{
	if (!AllChunks.Contains(C)) return;
	if (!AllNeighborsReady(C)) return;

	TSharedPtr<FChunkData> D = AllChunks.FindRef(C);
	if (!D.IsValid() || !D->bIsGenerated) return;

	EnqueueMesh(C);
}

bool UChunkManagerComponent::AllNeighborsReady(FIntPoint C) const
{
	for (int32 DY = -1; DY <= 1; DY++)
		for (int32 DX = -1; DX <= 1; DX++)
		{
			if (DX == 0 && DY == 0) continue;
			const TSharedPtr<FChunkData>* D = AllChunks.Find(FIntPoint(C.X + DX, C.Y + DY));
			if (!D || !D->IsValid() || !(*D)->bIsGenerated)
				return false;
		}
	return true;
}

void UChunkManagerComponent::DesiredCoords(const FIntPoint& Center, int32 R, TArray<FIntPoint>& Out) const
{
	FIntPoint CC = BlockToChunk(Center.X, Center.Y);
	Out.Reserve((2 * R + 1) * (2 * R + 1));
	for (int32 DY = -R; DY <= R; DY++)
		for (int32 DX = -R; DX <= R; DX++)
			if (DX * DX + DY * DY <= R * R)
				Out.Add(FIntPoint(CC.X + DX, CC.Y + DY));
}

TSharedPtr<FChunkData> UChunkManagerComponent::GetChunk(const FIntPoint& C) const
{
	const auto* Found = AllChunks.Find(C);
	return Found ? *Found : nullptr;
}

EBlockId UChunkManagerComponent::GetBlock(int32 BX, int32 BY, int32 BZ) const
{
	FIntPoint C = BlockToChunk(BX, BY);
	auto* D = AllChunks.Find(C);
	if (!D || !D->IsValid() || !(*D)->bIsGenerated) return EBlockId::Air;
	return (*D)->GetBlock(BX - C.X * CHUNK_SIZE, BY - C.Y * CHUNK_SIZE, BZ);
}

bool UChunkManagerComponent::SetBlock(int32 BX, int32 BY, int32 BZ, EBlockId Block)
{
	FIntPoint C = BlockToChunk(BX, BY);
	auto* D = AllChunks.Find(C);
	if (!D || !D->IsValid()) return false;

	(*D)->SetBlock(BX - C.X * CHUNK_SIZE, BY - C.Y * CHUNK_SIZE, BZ, Block);

	int32 LX = BX - C.X * CHUNK_SIZE;
	int32 LY = BY - C.Y * CHUNK_SIZE;

	bool bOnBorderX = (LX == 0 || LX == CHUNK_SIZE - 1);
	bool bOnBorderY = (LY == 0 || LY == CHUNK_SIZE - 1);

	EnqueueMesh(C);

	for (int32 DY = -1; DY <= 1; DY++)
		for (int32 DX = -1; DX <= 1; DX++)
		{
			if (DX == 0 && DY == 0) continue;
			if (DX != 0 && !bOnBorderX) continue;
			if (DY != 0 && !bOnBorderY) continue;
			EnqueueMesh(FIntPoint(C.X + DX, C.Y + DY));
		}

	return true;
}
