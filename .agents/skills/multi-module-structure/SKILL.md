---
name: multi-module-structure
description: "[MANDATORY — ALWAYS USE THIS SKILL] Enforces the strict 3-module UE5 architecture for Aethelgard. All new code must be placed in the correct module following the Public/Private convention with proper API macros. No file may violate the dependency DAG."
risk: safe
source: self
date_added: "2026-07-19"
---

# Multi-Module Structure — Aethelgard

This skill enforces the modular architecture of the Aethelgard project. Three independent UE5 modules, strict dependency DAG, Public/Private folder convention, API macros on all public classes.

## When to Use (IMPORTANT — ALWAYS)

**This skill is MANDATORY for EVERY code change.** Before writing ANY new file or modifying existing code, verify it respects the module boundaries below.

## Dependency DAG (NEVER VIOLATE)

```
AethelgardTerrain        (depends on: Engine only)
       ↑
AethelgardInteraction    (depends on: Engine + AethelgardTerrain)
       ↑
Aethelgard               (depends on: Engine + AethelgardTerrain + AethelgardInteraction)
```

| Module | Source dir | DLL | Owns |
|--------|-----------|-----|------|
| `AethelgardTerrain` | `Source/AethelgardTerrain/` | `AethelgardTerrain.dll` | Voxel world: chunks, greedy meshing, biome generation, networking, save system |
| `AethelgardInteraction` | `Source/AethelgardInteraction/` | `AethelgardInteraction.dll` | Inventory, building, item blocks |
| `Aethelgard` | `Source/Aethelgard/` | `Aethelgard.dll` | Game logic: Player, GameMode, GameState, HUD |

**Rules:**
- `AethelgardTerrain` NEVER includes anything from Interaction or Aethelgard
- `AethelgardInteraction` NEVER includes anything from Aethelgard
- `AethelgardInteraction` MAY include `AethelgardTerrain/X.h`
- `Aethelgard` MAY include anything from both sub-modules
- Circular dependencies are FORBIDDEN

## File Structure Convention

Every module follows the standard UE5 Public/Private layout:

```
Source/<ModuleName>/
├── <ModuleName>.Build.cs          ← Module dependencies
├── Public/<ModuleName>/            ← Headers (exported API)
│   ├── MyClass.h
│   └── ...
└── Private/                        ← Sources (internal)
    ├── <ModuleName>Module.cpp      ← IMPLEMENT_MODULE
    ├── MyClass.cpp
    └── ...
```

### Include paths

From any module, include public headers using the `ModuleName/Header.h` pattern:
```cpp
#include "AethelgardTerrain/ChunkData.h"
#include "AethelgardInteraction/InventoryComponent.h"
```

**NEVER use relative paths** like `"../Terrain/ChunkData.h"` or bare filenames for cross-module includes.

For `.generated.h` files, always use JUST the filename (no module prefix):
```cpp
#include "MyClass.generated.h"   // CORRECT
// #include "AethelgardTerrain/MyClass.generated.h"  // WRONG - UHT rejects this
```

The `.generated.h` include MUST be the LAST `#include` in the header.

## API Macros

Every class/struct/enum in `Public/<ModuleName>/` that is used by ANOTHER module needs the module's API macro:

```cpp
// AethelgardTerrain module
class AETHELGARDTERRAIN_API AVoxelWorld : public AActor { ... };
struct AETHELGARDTERRAIN_API FChunkData { ... };

// AethelgardInteraction module
class AETHELGARDINTERACTION_API UInventoryComponent : public UActorComponent { ... };
struct AETHELGARDINTERACTION_API FItemStack { ... };
```

| Module | API Macro |
|--------|-----------|
| `AethelgardTerrain` | `AETHELGARDTERRAIN_API` |
| `AethelgardInteraction` | `AETHELGARDINTERACTION_API` |
| `Aethelgard` | (none needed — no module depends on it) |

**Apply to:**
- All `UCLASS`, `USTRUCT`, `UENUM` in Public/ headers
- All plain C++ structs/classes with methods used across module boundaries
- All free functions (non-inline) declared in Public/ headers
- Inline functions defined in headers: add `static` or `FORCEINLINE`; the API macro is NOT needed for inline-only functions

**Do NOT apply to:**
- Files in `Private/` (they are only compiled within the module)
- Forward declarations (`class AVoxelWorld;`)
- Delegate declarations (`DECLARE_MULTICAST_DELEGATE`)
- `.generated.h` includes (UHT handles those)

## Adding a New Class

### Step-by-step

1. **Identify the right module** based on what the class depends on:
   - If it only uses Engine types → `AethelgardTerrain`
   - If it uses Terrain types (EBlockId, ChunkData, VoxelWorld) + Engine → `AethelgardInteraction`
   - If it uses Terrain + Interaction types → `Aethelgard`

2. **Create the header** in `Source/<Module>/Public/<Module>/NewClass.h`:
   ```cpp
   #pragma once
   #include "CoreMinimal.h"
   // ... other includes ...
   #include "NewClass.generated.h"   // ← LAST include, no module prefix

   UCLASS()
   class <MODULE>_API UNewClass : public UObject
   {
       GENERATED_BODY()
       // ...
   };
   ```

3. **Create the source** in `Source/<Module>/Private/NewClass.cpp`:
   ```cpp
   #include "<Module>/NewClass.h"    // ← module-relative path
   ```

4. **Verify the dependency DAG**: check every `#include` in your new file:
   - Can this file reach the included module following the DAG arrows?
   - Is there any upward reference (e.g., Interaction including Aethelgard)?

### Quick reference: where to put what

| Concept | Module | Example |
|---------|--------|---------|
| Block types, definitions | Terrain | `EBlockId`, `FBlockDefinition`, `GetBlockColor()` |
| Chunk data, world constants | Terrain | `FChunkData`, `CHUNK_SIZE`, `WORLD_HEIGHT` |
| World generation, biomes, noise | Terrain | `UWorldGeneratorComponent`, `FBiomeParams` |
| Mesh generation | Terrain | `UGreedyMeshGenerator`, `UVoxelMeshGenerator` |
| Save/load, chunk storage | Terrain | `USaveSystem`, `UChunkManagerComponent` |
| Networking | Terrain | `UNetworkSystemComponent` |
| Root world actor | Terrain | `AVoxelWorld` |
| Inventory, items | Interaction | `UInventoryComponent`, `UItemBlock` |
| Block placement/destruction | Interaction | `UBuildComponent` |
| Player, camera, movement | Aethelgard | `AAethelgardCharacter` |
| Game rules, console commands | Aethelgard | `AAethelgardGameMode` |
| HUD, debug overlay | Aethelgard | `AAethelgardHUD` |
| Trees, vegetation (future) | Terrain OR new module | — |
| Monsters, AI (future) | New module `AethelgardAI` | — |

## Adding a New Module (Future: AI, World, etc.)

1. Create `Source/<NewModule>/` with the standard structure
2. Write `<NewModule>.Build.cs` with its dependencies
3. Create `Private/<NewModule>Module.cpp` with `IMPLEMENT_MODULE`
4. Add the module to `Aethelgard.uproject`:
   ```json
   { "Name": "NewModule", "Type": "Runtime", "LoadingPhase": "Default" }
   ```
5. Add the dependency in the consuming module's `Build.cs`
6. Ensure the dependency arrow is downward in the DAG (no cycles)

## Build.cs Template

```csharp
using UnrealBuildTool;

public class ModuleName : ModuleRules
{
    public ModuleName(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine",
            // Add module dependencies here (e.g. "AethelgardTerrain")
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "AssetTools", "UnrealEd"
            });
        }
    }
}
```

Notes:
- Use `PublicDependencyModuleNames` for modules whose headers appear in YOUR public headers
- Use `PrivateDependencyModuleNames` for modules only used in your `.cpp` files
- Editor-only modules (`AssetTools`, `UnrealEd`) go in `PrivateDependencyModuleNames` under the `bBuildEditor` guard
- `PrivateIncludePaths.Add(ModuleDirectory)` is NOT needed — UBT discovers source files automatically

## Checklist

Before committing any code:

- [ ] Is every new file in the correct module directory?
- [ ] Do all `#include` paths use the `ModuleName/Header.h` format?
- [ ] Are `.generated.h` includes bare filenames (no module prefix)?
- [ ] Are `.generated.h` includes the LAST `#include` in each header?
- [ ] Do all public classes/structs have the correct `<MODULE>_API` macro?
- [ ] Does every new include respect the dependency DAG (no upward references)?
- [ ] Is the `.uproject` up-to-date with all modules?
- [ ] Does `BuildAethelgard.bat` pass with zero errors?

## Rationale

- **Reusability**: `AethelgardTerrain` can be copy-pasted to another UE project and work standalone
- **Compile times**: Changes to one module don't trigger recompilation of others (unless headers change)
- **Dependency hygiene**: The DAG prevents spaghetti code — you always know what can depend on what
- **Team scaling**: Different developers can work on different modules without stepping on each other
- **UE5 standard**: Multi-module structure is how Epic builds the engine itself and how large projects like Lyra are organized
