# AGENTS.md — Aethelgard

## Build

UE 5.8, C++ only. No Blueprint.

```bash
# From WSL (preferred)
cmd.exe /c "BuildAethelgard.bat"

# Or directly
cmd.exe /c "\"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat\" AethelgardEditor Development Win64 \"C:\Users\herilala\Documents\Unreal Projects\Aethelgard\Aethelgard.uproject\" -NoEngineChanges"
```

Logs: `Build.log` at project root. Build is a DLL loaded by `UnrealEditor.exe`.

## Architecture

Single module `Aethelgard`, two subdirectories:
- `Terrain/` — Voxel world: chunks, greedy meshing, biome generation, networking, save system
- `Game/` — Player, GameMode, GameState, HUD

Key classes:
- `AVoxelWorld` — Root actor, owns components, one `UProceduralMeshComponent` per chunk (`ChunkMeshes` map)
- `UChunkManagerComponent` — Chunk storage, load/unload around player, view distance
- `UWorldGeneratorComponent` — Biome system, height generation, geological layers
- `UGreedyMeshGenerator` — Greedy meshing, outputs `TMap<EBlockId, FMeshSectionData>` (multi-section)
- `AethelgardHUD` — Debug overlay (position, chunk, biome, seed, FPS)

## Constants

- `BlockScale = 100.0f` (world units per block)
- `CHUNK_SIZE = 32` (blocks per chunk axis)
- `WORLD_HEIGHT = 256`
- `WaterLevel = 35.0f`

## Materials

Defined in `BlockRegistry.h` as `FBlockDefinition::MaterialPath`. Auto-created on editor play via `AVoxelWorld::EnsureBlockMaterialsExist()` which duplicates `M_Default` for each block type.

**File locations** (must match code paths):
- Source material: `Content/Materials/M_Default.uasset`
- Block materials: `Content/Materials/Environment/` (Stone, Dirt, Grass, Sand, Wood, Leaves) and `Content/Materials/Liquid/` (Water)

The `WITH_EDITOR` guard means `EnsureBlockMaterialsExist()` only runs in editor. Shipping builds require materials to exist on disk.

## Build.cs

Editor-only modules `AssetTools` and `UnrealEd` are added conditionally:
```csharp
if (Target.bBuildEditor)
    PrivateDependencyModuleNames.AddRange(new string[] { "AssetTools", "UnrealEd" });
```

## Key files

| File | What it defines |
|------|----------------|
| `BlockRegistry.h` | `EBlockId` enum, `FBlockDefinition`, `GetBlockDef()`, `GetBlockColor()` |
| `ChunkData.h` | `FChunkData` struct |
| `VoxelWorld.h/.cpp` | Per-chunk mesh components, material loading, tick loop |
| `WorldGeneratorComponent.h/.cpp` | Biome params, noise, height computation, lake generation |
| `GreedyMeshGenerator.h/.cpp` | Greedy mesh output per block type |
| `SaveSystem.h/.cpp` | `USaveGame` subclass, block changes + inventory serialization via `UGameplayStatics` |
| `InventoryComponent.h/.cpp` | 36-slot inventory, `AddItem`/`RemoveItem`, save/load via `FInventorySlotSaveData` |
| `ItemBlock.h` | `UItemBlock` UObject (BlockId, DisplayName, MaxStack, TintColor) |
| `AethelgardHUD.h/.cpp` | Debug HUD overlay |

## Pitfalls

- `GetBlock()` / `SetBlock()` on `AVoxelWorld` take **world** coords and divide by `BlockScale`. `ChunkManagerComponent` uses **chunk-local** coords.
- Mesh sections are assigned per block type. `ClearSection` destroys the chunk's `UProceduralMeshComponent` entirely, removing both mesh and collision.
- The `Async()` call in `BuildSection` captures `this` — ensure `IsValid(this)` check on game thread callback.
- `FBlockDefinition::MaterialPath` must start with `/Game/` (UE package path convention).
- Inventory is saved as `TArray<FInventorySlotSaveData>` (BlockId + Quantity pairs), NOT as UObject pointers. `LoadFromSaveData` recreates `UItemBlock` objects from the saved data.
- `SaveGame`/`LoadGame` are `UFUNCTION(Exec)` — callable from console. GameMode orchestrates: gets pawn's inventory, passes through VoxelWorld to SaveSystem.
- French language in game description/UI — keep comments/code in English, game text can be French.
