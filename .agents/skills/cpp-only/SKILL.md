---
name: cpp-only
description: "[MANDATORY — ALWAYS USE THIS SKILL] Strict policy: no Blueprints — all logic must be authored in pure C++, leveraging Unreal Engine's native C++ APIs to the fullest extent."
risk: safe
source: self
date_added: "2026-07-16"
---

# C++ Only — No Blueprints

This skill enforces a hard requirement for this project: **never use Blueprints**. All gameplay logic, systems, and interactions must be implemented in pure C++.

## When to Use (IMPORTANT — ALWAYS)

**This skill is MANDATORY and must be used for EVERY task in this project, without exception.**

Use this skill when:
- Writing any game logic, systems, AI, UI, or gameplay code
- Defining Actors, Components, Interfaces, or any UObject-derived classes
- Implementing gameplay events, timers, delegates, or replication
- Exposing properties or functions for editor configuration (via `UPROPERTY` / `UFUNCTION`)
- Creating data assets or struct definitions

## Core Policy

### 1. No Blueprint Logic
- **Zero tolerance for Blueprint graphs.** Do not create, modify, or rely on EventGraphs, Construction Scripts, Function Graphs, or Macro Graphs.
- Blueprint classes should not contain any logic — they may exist only as C++-defined classes that are optionally selectable in the editor for convenience (e.g., `Blueprintable` for level placement).
- All control flow, math, spawning, timers, and event handling must be done in C++.

### 2. Use Unreal's Native C++ APIs
- Prefer Unreal Engine's built-in C++ systems over custom implementations:
  - `UCLASS()`, `USTRUCT()`, `UENUM()`, `UFUNCTION()`, `UPROPERTY()` for reflection
  - `FTimerHandle` / `GetWorldTimerManager()` for timed logic
  - `FGameplayTag` for tagging and querying
  - `TArray`, `TMap`, `TSet`, `TObjectPtr`, `TStrongObjectPtr` for containers
  - `FDelegateHandle`, `DECLARE_DYNAMIC_MULTICAST_DELEGATE` for event dispatching
  - `GameplayAbilities` / `GameplayEffects` if using the GameplayAbilitySystem
  - `EnhancedInput` for input handling (via `UInputMappingContext` and `UInputAction` in C++)
  - `UPrimitiveComponent`, `USceneComponent`, `UActorComponent` for component composition
  - `UGameplayStatics` for common game utilities (spawning, opening levels, etc.)
- Use UE's reflection system to make properties editable in the editor via `EditAnywhere`, `EditDefaultsOnly`, `Category`, etc., so designers can tune values without writing Blueprint logic.

### 3. C++ First — Editor Configurable
- Use `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "...")` liberally so designers can configure values in the Editor without needing Blueprint graphs.
- Use `UFUNCTION(BlueprintCallable)` only when you need to expose a callable interface for the editor (e.g., for `CallInEditor` functions). Otherwise keep them as plain C++ functions.
- Use `UDataAsset` or `UPrimaryDataAsset` for data-driven design instead of Blueprint-editable variables inside Blueprint classes.

### 4. What About UI?
- Use Slate (C++ widget framework) or `UUserWidget` subclasses with all logic in C++ (`NativeTick`, `NativeOnInitialized`, binding events in C++).
- Do not use Blueprint widgets or event graphs in UMG.

### 5. What About Animations?
- Use `UAnimInstance` C++ subclasses. Logic for state machines, blend spaces, and notify handling must be in C++.
- Use `FAnimNode` / anim node context in C++ for custom animation nodes if needed.

### 6. Exceptions
- None. If you find yourself tempted to write a Blueprint graph, stop and write C++ instead.

## Checklist

Before considering any implementation complete:

- [ ] Is there any Blueprint graph created or modified? If yes, **rewrite it in C++**.
- [ ] Could this logic have been written in C++ using Unreal's native APIs? If yes, it must be.
- [ ] Are `UPROPERTY` / `UFUNCTION` annotations used appropriately to expose configuration to the editor?
- [ ] Is the project reliant on Blueprint logic anywhere? If so, migrate it to C++.

## Rationale

- **Performance**: C++ avoids the overhead of Blueprint VM execution.
- **Maintainability**: C++ code is easier to refactor, diff, review, and version control.
- **Reliability**: Compile-time type checking catches errors early.
- **Consistency**: A single language across the entire codebase reduces context switching.
