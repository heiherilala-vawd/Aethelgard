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

Three modules, strict dependency DAG:

```
AethelgardTerrain (no deps except Engine)
       ↑
AethelgardInteraction (depends on Terrain)
       ↑
Aethelgard (main module, depends on both)
```

| Module | Source dir | DLL | Purpose |
|--------|-----------|-----|---------|
| `AethelgardTerrain` | `Source/AethelgardTerrain/` | `AethelgardTerrain.dll` | Voxel world: chunks, greedy meshing, biome generation, networking, save system |
| `AethelgardInteraction` | `Source/AethelgardInteraction/` | `AethelgardInteraction.dll` | Inventory, building, item blocks |
| `Aethelgard` | `Source/Aethelgard/` | `Aethelgard.dll` | Game logic: Player, GameMode, GameState, HUD |

Each module follows UE convention: `Public/<ModuleName>/` for headers, `Private/` for sources.
Public classes carry `<MODULE>_API` macro (e.g. `AETHELGARDTERRAIN_API`).

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

| File | Module | What it defines |
|------|--------|----------------|
| `Public/AethelgardTerrain/BlockRegistry.h` | Terrain | `EBlockId` enum, `FBlockDefinition`, `GetBlockDef()`, `GetBlockColor()` |
| `Public/AethelgardTerrain/ChunkData.h` | Terrain | `FChunkData` struct, `CHUNK_SIZE`, `WORLD_HEIGHT` |
| `Public/AethelgardTerrain/VoxelWorld.h/.cpp` | Terrain | Per-chunk mesh components, material loading, tick loop |
| `Public/AethelgardTerrain/WorldGeneratorComponent.h/.cpp` | Terrain | Biome params, noise, height computation |
| `Public/AethelgardTerrain/GreedyMeshGenerator.h/.cpp` | Terrain | Greedy mesh output per block type |
| `Public/AethelgardTerrain/SaveSystem.h/.cpp` | Terrain | `USaveGame` subclass, block changes + inventory serialization |
| `Public/AethelgardInteraction/InventoryComponent.h/.cpp` | Interaction | 36-slot inventory, `AddItem`/`RemoveItem`, save/load |
| `Public/AethelgardInteraction/ItemBlock.h` | Interaction | `UItemBlock` UObject (BlockId, DisplayName, MaxStack, TintColor) |
| `Public/AethelgardInteraction/BuildComponent.h/.cpp` | Interaction | Block placement/destruction via trace |
| `Game/AethelgardHUD.h/.cpp` | Main | Debug HUD overlay |
| `Game/AethelgardGameMode.h/.cpp` | Main | Console commands `SaveGame`/`LoadGame` |
| `Game/AethelgardCharacter.h/.cpp` | Main | First-person character, owns build + inventory components |

## Pitfalls

- `GetBlock()` / `SetBlock()` on `AVoxelWorld` take **world** coords and divide by `BlockScale`. `ChunkManagerComponent` uses **chunk-local** coords.
- Mesh sections are assigned per block type. `ClearSection` destroys the chunk's `UProceduralMeshComponent` entirely, removing both mesh and collision.
- The `Async()` call in `BuildSection` captures `this` — ensure `IsValid(this)` check on game thread callback.
- `FBlockDefinition::MaterialPath` must start with `/Game/` (UE package path convention).
- Inventory is saved as `TArray<FInventorySlotSaveData>` (BlockId + Quantity pairs), NOT as UObject pointers. `LoadFromSaveData` recreates `UItemBlock` objects from the saved data.
- `SaveGame`/`LoadGame` are `UFUNCTION(Exec)` — callable from console. GameMode orchestrates: gets pawn's inventory, passes through VoxelWorld to SaveSystem.
- French language in game description/UI — keep comments/code in English, game text can be French.
