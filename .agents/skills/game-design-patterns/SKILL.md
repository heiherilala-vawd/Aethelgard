---
name: game-design-patterns
description: "[MANDATORY — ALWAYS USE THIS SKILL] Comprehensive catalog of all GoF design patterns and Game Programming Patterns, with Unreal Engine 5 C++ implementations for Aethelgard."
risk: safe
source: self
date_added: "2026-07-16"
---

# Game Design Patterns

This skill catalogs **all Gang of Four (GoF) design patterns** and **Game Programming Patterns** (Robert Nystrom), mapped to Unreal Engine 5 C++ and the Aethelgard architecture.

## How to Use

- **Always consult this skill** before implementing any non-trivial system.
- **Prefer Game Programming Patterns** when they fit the game domain.
- **Fall back to GoF patterns** when no game-specific pattern matches.
- **Prefer Unreal-native idioms** (e.g., `FTimerManager`, `TArray`, `UCLASS`, `UPROPERTY`) over boilerplate pattern implementations.
- **Always apply the simplest correct pattern** — do not over-engineer.

---

# Part 1: Creational Patterns (GoF)

## 1. Factory Method

**When**: You need to create objects without specifying the exact class. The exact type is determined at runtime.

**UE5 Idiom**: `UClass*` + `NewObjectDeferred<>()` / `SpawnActorDeferred<>()`. Use `TSubclassOf<>` for type selection.

```cpp
UCLASS()
class UEntityFactory : public UObject
{
    GENERATED_BODY()

public:
    ALivingEntity* SpawnEntity(UWorld* World, TSubclassOf<ALivingEntity> EntityClass, const FTransform& Transform)
    {
        FActorSpawnParameters Params;
        return World->SpawnActorDeferred<ALivingEntity>(EntityClass, Transform);
    }
};
```

**In Aethelgard**: `ASpawnManager::SpawnEntity()` in UML. Use factory methods per entity category (creatures, NPCs, structures).

---

## 2. Abstract Factory

**When**: You need families of related objects that must work together (e.g., different biomes produce different resources, creatures, and structures).

**UE5 Idiom**: `UDataAsset` subclasses that each produce a family of `TSubclassOf<>` references.

```cpp
UCLASS()
class UBiomeFactory : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TSubclassOf<ALivingEntity> PreyClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<ALivingEntity> PredatorClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AStructureBase> ResourceNodeClass;

    UPROPERTY(EditAnywhere)
    TMap<EMaterialType, TSubclassOf<AItemPickup>> LootTable;
};
```

**In Aethelgard**: Each `UBiome` data asset acts as an abstract factory for that biome's entity ecosystem.

---

## 3. Builder

**When**: Object construction has many optional parameters, or the construction process has multiple steps.

**UE5 Idiom**: Construction script in C++ via `SpawnActorDeferred` + `FinishSpawning`. Fluent builders on `FTransform` or `FActorSpawnParameters`.

```cpp
class FEntityBuilder
{
public:
    FEntityBuilder& WithClass(TSubclassOf<ALivingEntity> InClass) { Class = InClass; return *this; }
    FEntityBuilder& WithTransform(const FTransform& InTransform) { Transform = InTransform; return *this; }
    FEntityBuilder& WithControllerClass(TSubclassOf<AController> InCtrl) { ControllerClass = InCtrl; return *this; }

    ALivingEntity* Build(UWorld* World)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        return World->SpawnActorDeferred<ALivingEntity>(Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::Undefined);
    }

private:
    TSubclassOf<ALivingEntity> Class;
    FTransform Transform = FTransform::Identity;
    TSubclassOf<AController> ControllerClass;
};
```

---

## 4. Prototype

**When**: Creating a copy is cheaper or more flexible than creating from scratch. You need to override specific properties while inheriting the rest.

**UE5 Idiom**: `UDataAsset` inheritance with child-override pattern (`UPROPERTY` overrides). Use `DuplicateObject()` for runtime copies.

```cpp
// Parent data asset defines defaults
UPROPERTY(EditAnywhere, Category = "Default")
float BaseHealth = 100.0f;

// Child can override specific values
UPROPERTY(EditAnywhere, Category = "Override", meta = (EditCondition = "bOverrideHealth"))
float HealthOverride = 150.0f;
```

**In Aethelgard**: `URaceData.ParentRace`, `UBodyPlan.ParentPlan`, `UItemBase.ParentItem`. The entire data-driven inheritance system is a **Template Method + Prototype** hybrid. Any data asset can inherit from a parent and override fields.

---

## 5. Singleton

**When**: Exactly one instance of a class must exist globally.

**UE5 Idiom**: Do NOT implement Singleton manually. Use Unreal's global singletons: `UGameInstance` (per-game, persistent), `AGameMode` (per-level authority), `UGameplayStatics` (static helpers). For custom singletons, prefer `UGameInstanceSubsystem`.

```cpp
UCLASS()
class USpawnManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    ALivingEntity* SpawnEntity(TSubclassOf<ALivingEntity> Class, const FTransform& Transform);

    static USpawnManagerSubsystem* Get(const UObject* WorldContext)
    {
        if (UGameInstance* GI = WorldContext->GetWorld()->GetGameInstance())
            return GI->GetSubsystem<USpawnManagerSubsystem>();
        return nullptr;
    }
};
```

**Rule**: Never use static `Get()` / `Instance()` singletons. Always use `UGameInstanceSubsystem`, `UWorldSubsystem`, or `UEngineSubsystem`.

**In Aethelgard**: `ASpawnManager`, `AGameMode`, `AGameState` — all are Unreal-native singletons.

---

# Part 2: Structural Patterns (GoF)

## 6. Adapter

**When**: You need to make an existing interface work with another incompatible interface.

**UE5 Idiom**: Wrapper functions, interface implementations on third-party systems, or `FCppAdapter` classes.

```cpp
// Third-party library interface
class FExternalPhysicsAPI
{
public:
    void ApplyForceRaw(int32 BodyId, float X, float Y, float Z);
};

// Unreal-compatible adapter
class FExternalPhysicsAdapter
{
public:
    void ApplyForceToComponent(UPrimitiveComponent* Comp, FVector Force)
    {
        int32 BodyId = Comp->GetBodyInstance()->BodyIndex;
        Adapter.ApplyForceRaw(BodyId, Force.X, Force.Y, Force.Z);
    }

private:
    FExternalPhysicsAPI Adapter;
};
```

---

## 7. Bridge

**When**: You need to decouple an abstraction from its implementation so they can vary independently.

**UE5 Idiom**: Interface (`UInterface`) + separate implementation classes. In UE5, this maps naturally to `TScriptInterface<>` or direct interface casts.

```cpp
UINTERFACE()
class UDamageHandler : public UInterface
{
    GENERATED_BODY()
};

class IDamageHandler
{
    GENERATED_BODY()

public:
    virtual void HandleDamage(FDamageEvent& Event) = 0;
};

// Bridge: different entity types implement HandleDamage differently
class ALivingEntity : public AActor, public IDamageHandler { ... };
class AStructureBase : public AActor, public IDamageHandler { ... };
```

**In Aethelgard**: Bridge pattern is pervasive via Unreal Interfaces (`IInheritableDefinition`, `ITakeDamage`, `IHasBodyParts`, `IFlying`, `ISwimming`, `IMountable`).

---

## 8. Composite

**When**: Objects must be treated as individual elements or as collections of elements uniformly (tree structures).

**UE5 Idiom**: `AActor` / `UActorComponent` hierarchy is inherently composite. Also `TArray<>` of child elements with a common base.

```cpp
USTRUCT()
struct FBodyPartDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName PartName;

    UPROPERTY(EditAnywhere)
    TArray<FBodyPartDefinition> SubParts;

    UPROPERTY(EditAnywhere)
    float MaxHealth;

    float GetTotalHealth() const
    {
        float Total = MaxHealth;
        for (const FBodyPartDefinition& Sub : SubParts)
            Total += Sub.GetTotalHealth();
        return Total;
    }
};
```

**In Aethelgard**: `UBodyPlan` → `FBodyPartDefinition[]` (body parts can have sub-parts). `USkillTree` → `FSkillNode[]`. `URecipe` → `FRecipeIngredient[]`. `ULootTable` → `FLootEntry[]`.

---

## 9. Decorator

**When**: You need to add responsibilities to objects dynamically without subclassing.

**UE5 Idiom**: `UStatusEffectComponent` with a stack of modifiers that wrap the base value. Each effect decorates the base stats.

```cpp
UCLASS()
class UAttributeModifier : public UObject
{
    GENERATED_BODY()

public:
    virtual float ModifyValue(float BaseValue) const { return BaseValue; }
};

UCLASS()
class UAdditiveModifier : public UAttributeModifier
{
    GENERATED_BODY()

public:
    UPROPERTY()
    float Addend = 0.0f;

    virtual float ModifyValue(float BaseValue) const override
    {
        return BaseValue + Addend;
    }
};

UCLASS()
class UMultiplicativeModifier : public UAttributeModifier
{
    GENERATED_BODY()

public:
    UPROPERTY()
    float Multiplier = 1.0f;

    virtual float ModifyValue(float BaseValue) const override
    {
        return BaseValue * Multiplier;
    }
};
```

---

## 10. Facade

**When**: You need a simplified interface to a complex subsystem.

**UE5 Idiom**: `ALivingEntity` is a Facade over ~15 components. `UGameplayStatics` is Unreal's own Facade over world operations.

```cpp
UCLASS()
class ALivingEntity : public AActor
{
    GENERATED_BODY()

public:
    // Facade methods that delegate to internal components
    void TakeDamage(float Amount, FDamageEvent Event)
    {
        CombatComponent->ApplyDamage(Amount, Event);
    }

    void AddStatusEffect(TSubclassOf<UStatusEffect> EffectClass)
    {
        StatusEffectComponent->ApplyEffect(EffectClass);
    }

    void MoveToLocation(const FVector& Dest)
    {
        BehaviorComponent->MoveTo(Dest);
    }

private:
    UPROPERTY()
    UCombatComponent* CombatComponent;

    UPROPERTY()
    UStatusEffectComponent* StatusEffectComponent;

    UPROPERTY()
    UBehaviorComponent* BehaviorComponent;

    // ... ~12 more components
};
```

**In Aethelgard**: `ALivingEntity` is explicitly designed as a Facade. See UML line 759–801.

---

## 11. Flyweight

**When**: Many fine-grained objects share common data. Memory optimization.

**UE5 Idiom**: `UDataAsset` for shared immutable state. `TSoftObjectPtr<>` to avoid loading duplicates. `FName` / `FString` interning.

```cpp
// Flyweight: shared data asset
UCLASS()
class UItemTemplate : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FText DisplayName;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere)
    float Weight;
};

// Use: thousands of item instances share the same template
USTRUCT()
struct FItemStack
{
    GENERATED_BODY()

    UPROPERTY()
    UItemTemplate* Template;

    UPROPERTY()
    int32 Count;
};
```

**In Aethelgard**: Use for item stacks, block types in the voxel world, status effect templates, and any object where thousands of instances share identical data.

---

## 12. Proxy

**When**: You need a placeholder for another object to control access, lazy loading, or networking.

**UE5 Idiom**: `TSoftObjectPtr<T>` — a proxy that loads the actual asset on demand. Network Replication is a form of proxy (client-side actors proxy server state). `TWeakObjectPtr<>` / `TLazyObjectPtr<>`.

```cpp
// Lazy-loading proxy for expensive assets
UPROPERTY(EditAnywhere)
TSoftObjectPtr<USkeletalMesh> DragonMesh;

void ALivingEntity::LoadMesh()
{
    if (DragonMesh.IsPending())
    {
        DragonMesh.LoadSynchronous();
    }
    GetMesh()->SetSkeletalMesh(DragonMesh.Get());
}
```

**In Aethelgard**: Use `TSoftObjectPtr` for all asset references in data assets to avoid load chains. Used extensively in the architecture via `IInheritableDefinition` system.

---

# Part 3: Behavioral Patterns (GoF)

## 13. Chain of Responsibility

**When**: A request should be passed through a chain of handlers until one handles it.

**UE5 Idiom**: `UActorComponent` chain, event bubbling, or custom handler chains. UE's input system (`UInputComponent::BindAction`) is a CoR.

```cpp
class UDamageHandlerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY()
    UDamageHandlerComponent* NextHandler;

    virtual void HandleDamage(FDamageEvent& Event)
    {
        if (!bCanHandle(Event) && NextHandler)
        {
            NextHandler->HandleDamage(Event);
        }
    }

protected:
    virtual bool bCanHandle(const FDamageEvent& Event) { return false; }
};

// Chain: ArmorHandler -> StatusEffectHandler -> BaseHealthHandler
```

---

## 14. Command

**When**: You need to parameterize, queue, log, or undo actions.

**UE5 Idiom**: `UAbilityInstance` with `Start()` / `Interrupt()`. `FActorSpawnParameters`. Enhanced Input actions. UObject-based commands.

```cpp
UCLASS()
class UCommand : public UObject
{
    GENERATED_BODY()

public:
    virtual void Execute() PURE_VIRTUAL(UCommand::Execute, );
    virtual void Undo() PURE_VIRTUAL(UCommand::Undo, );
};

UCLASS()
class USpawnEntityCommand : public UCommand
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    AActor* SpawnedActor;

    UPROPERTY()
    FTransform Transform;

    virtual void Execute() override
    {
        SpawnedActor = GetWorld()->SpawnActorDeferred<AActor>(ActorClass, Transform);
        if (SpawnedActor) SpawnedActor->FinishSpawning(Transform);
    }

    virtual void Undo() override
    {
        if (SpawnedActor) SpawnedActor->Destroy();
    }
};
```

**In Aethelgard**: `UAbilityInstance` with `Start()` / `Interrupt()` is a Command pattern (UML lines 316–323). Use for ability system, input actions, and queuable player actions.

---

## 15. Iterator

**When**: You need sequential access to elements of a collection without exposing the underlying representation.

**UE5 Idiom**: `TArray` iterators, `TIterator<>`, ranged-for. Use `CreateIterator()` / `CreateConstIterator()` on `TMap`, `TSet`.

```cpp
TArray<FBodyPartDefinition> BodyParts;
for (FBodyPartDefinition& Part : BodyParts)
{
    ProcessPart(Part);
}

// Or more explicit
for (auto It = BodyParts.CreateIterator(); It; ++It)
{
    ProcessPart(*It);
}
```

**In Aethelgard**: Standard C++ iterators on all containers. No custom iterator needed.

---

## 16. Mediator

**When**: Complex interactions between multiple objects should be centralized rather than having objects reference each other directly.

**UE5 Idiom**: `AGameMode` as mediator between players, AI, and world state. `ALivingEntity` as mediator between its components.

```cpp
UCLASS()
class ACombatMediator : public AActor
{
    GENERATED_BODY()

public:
    void OnEntityDamaged(ALivingEntity* Target, AActor* Instigator, float Amount)
    {
        // Notify all interested systems without coupling them directly
        OnDamageDelegates.Broadcast(Target, Instigator, Amount);
        UpdateThreatLevels(Target, Instigator);
        CheckBossPhaseTransition(Target);
        SpawnLootIfDead(Target, Instigator);
    }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEntityDamaged, ALivingEntity*, Target, AActor*, Instigator, float, Amount);
    FOnEntityDamaged OnDamageDelegates;

private:
    void UpdateThreatLevels(ALivingEntity* Target, AActor* Instigator);
    void CheckBossPhaseTransition(ALivingEntity* Target);
    void SpawnLootIfDead(ALivingEntity* Target, AActor* Instigator);
};
```

**In Aethelgard**: Mediator pattern useful for death → loot → XP → quest update → respawn flow. Can be implemented as a `UGameInstanceSubsystem` or level-specific mediator actor.

---

## 17. Memento

**When**: You need to capture and restore an object's internal state (save/load, undo).

**UE5 Idiom**: `USaveGame` subclasses + `UGameplayStatics::SaveGameToSlot()`. `FEntitySaveData` structs.

```cpp
UCLASS()
class UEntityMemento : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FRotator Rotation;

    UPROPERTY()
    float Health;

    UPROPERTY()
    float Hunger;

    UPROPERTY()
    TArray<FItemStack> Inventory;
};

void ALivingEntity::CreateMemento()
{
    UEntityMemento* Save = Cast<UEntityMemento>(UGameplayStatics::CreateSaveGameObject(UEntityMemento::StaticClass()));
    Save->Location = GetActorLocation();
    Save->Health = CombatComponent->GetHealth();
    UGameplayStatics::SaveGameToSlot(Save, GetFName().ToString(), 0);
}
```

**In Aethelgard**: Currently uses `FEntitySaveData` struct (Memento). Formalize via `USaveGame` subclasses for persistence.

---

## 18. Observer

**When**: One object should notify many others about state changes without being coupled to them.

**UE5 Idiom**: `DECLARE_DYNAMIC_MULTICAST_DELEGATE` / `DECLARE_MULTICAST_DELEGATE`. UE's event system is the canonical Observer.

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, OldHealth, float, NewHealth);

UCLASS()
class UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    void ApplyDamage(float Amount)
    {
        float OldHealth = Health;
        Health = FMath::Max(0.0f, Health - Amount);
        OnHealthChanged.Broadcast(OldHealth, Health);
    }

private:
    float Health = 100.0f;
};
```

**In Aethelgard**: Use for health changes, status effect ticks, XP gain, inventory updates, ability cooldowns. Prefer `DECLARE_DYNAMIC_MULTICAST_DELEGATE` for replication support.

---

## 19. State

**When**: An object's behavior depends on its internal state, and that state changes at runtime.

**UE5 Idiom**: `UBehaviorComponent` with state machine. `UAnimInstance` state machines. `EBossPhase` enum + phase switch.

```cpp
UENUM()
enum class ELivingEntityState : uint8
{
    Idle,
    Patrolling,
    Chasing,
    Attacking,
    Fleeing,
    Dead
};

UCLASS()
class UBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void SetState(ELivingEntityState NewState)
    {
        if (CurrentState == NewState) return;
        ExitState(CurrentState);
        CurrentState = NewState;
        EnterState(CurrentState);
    }

    void TickState(float DeltaTime)
    {
        switch (CurrentState)
        {
        case ELivingEntityState::Idle: TickIdle(DeltaTime); break;
        case ELivingEntityState::Chasing: TickChasing(DeltaTime); break;
        case ELivingEntityState::Attacking: TickAttacking(DeltaTime); break;
        // ...
        }
    }

private:
    ELivingEntityState CurrentState = ELivingEntityState::Idle;

    void EnterState(ELivingEntityState State);
    void ExitState(ELivingEntityState State);
    void TickIdle(float DeltaTime);
    void TickChasing(float DeltaTime);
    void TickAttacking(float DeltaTime);
};
```

**In Aethelgard**: `UBossComponent` + `FBossPhase` with health thresholds and phase transitions (UML lines 610–627). Also used for AI behavior states, animation states, and interaction states.

---

## 20. Strategy

**When**: You need to select an algorithm at runtime from a family of algorithms.

**UE5 Idiom**: Interface + implementation swapping. `TUniquePtr<IStrategy>` or `UObject*` base class pointer.

```cpp
UINTERFACE()
class UDamageCalculation : public UInterface
{
    GENERATED_BODY()
};

class IDamageCalculation
{
    GENERATED_BODY()

public:
    virtual float CalculateDamage(float BaseDamage, const FDamageContext& Context) = 0;
};

// Strategies
class FPhysicalDamageCalc : public IDamageCalculation
{
    virtual float CalculateDamage(float BaseDamage, const FDamageContext& Context) override
    {
        return BaseDamage * (1.0f - Context.Armor / 100.0f);
    }
};

class FMagicalDamageCalc : public IDamageCalculation
{
    virtual float CalculateDamage(float BaseDamage, const FDamageContext& Context) override
    {
        return BaseDamage * (1.0f - Context.MagicResist / 100.0f);
    }
};
```

**In Aethelgard**: `UAbilityEffect` (abstract) → `UDamageEffect`, `UHealEffect`, `UStatusApplyEffect`, `USummonEffect`, `UTeleportEffect` (UML lines 341–369). The entire effect system is Strategy-based.

---

## 21. Template Method

**When**: An algorithm has an invariant structure but some steps can be overridden by subclasses.

**UE5 Idiom**: Virtual functions called in a fixed order. `UAnimInstance::NativeUpdateAnimation()`. `AActor::BeginPlay()`.

```cpp
UCLASS()
class AEntityBase : public AActor
{
    GENERATED_BODY()

public:
    // Template Method — skeleton defines the spawning flow
    void InitializeEntity(const FEntityContext& Context)
    {
        OnPreInitialize(Context);
        ApplyBaseStats(Context);
        ApplyRaceModifiers(Context);
        InitializeComponents(Context);
        OnPostInitialize(Context);
    }

protected:
    // Steps — overridable
    virtual void OnPreInitialize(const FEntityContext& Context) {}
    virtual void ApplyBaseStats(const FEntityContext& Context);
    virtual void ApplyRaceModifiers(const FEntityContext& Context);
    virtual void InitializeComponents(const FEntityContext& Context);
    virtual void OnPostInitialize(const FEntityContext& Context) {}
};
```

**In Aethelgard**: `IInheritableDefinition::GetEffectiveX()` pattern — parent data assets define defaults, children override specific values (UML lines 99–103). This is the foundation of the entire data inheritance system.

---

## 22. Visitor

**When**: You need to perform operations on all elements of a complex object structure without changing their classes.

**UE5 Idiom**: Iterate + switch/dynamic cast. In UE5, prefer interfaces over the classic Visitor pattern.

```cpp
class IBodyPartVisitor
{
public:
    virtual void VisitHead(class UHeadPart* Part) = 0;
    virtual void VisitTorso(class UTorsoPart* Part) = 0;
    virtual void VisitLimb(class ULimbPart* Part) = 0;
};

class UDamageApplicationVisitor : public UObject, public IBodyPartVisitor
{
    GENERATED_BODY()

public:
    virtual void VisitHead(UHeadPart* Part) override
    {
        // Apply critical damage multiplier
        Part->ApplyRawDamage(Amount * 2.0f);
    }

    virtual void VisitTorso(UTorsoPart* Part) override
    {
        Part->ApplyRawDamage(Amount * 1.0f);
    }

    virtual void VisitLimb(ULimbPart* Part) override
    {
        Part->ApplyRawDamage(Amount * 0.5f);
    }

    float Amount;
};
```

**In Aethelgard**: Useful for processing all body parts uniformly (area damage, healing, status effect application). The body part system (`UBodyPlan` + `FBodyPartDefinition[]`) benefits from Visitor-style traversal.

---

# Part 4: Game Programming Patterns

## 23. Game Loop

**When**: You need to control the pacing and update order of game systems.

**UE5 Idiom**: **Do not implement**. Unreal Engine's `UGameEngine::Tick()` already implements a game loop (input → tick → rendering). Use UE's tick groups (`TG_PrePhysics`, `TG_DuringPhysics`, `TG_PostPhysics`, `TG_LastDemotable`) to control update order.

```cpp
// Set tick group to control update ordering
PrimaryActorTick.TickGroup = TG_PrePhysics;

void ALivingEntity::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Custom update logic at the correct phase
}
```

---

## 24. Update Method

**When**: Each entity needs to update itself every frame (per-frame behavior).

**UE5 Idiom**: Override `AActor::Tick(float DeltaTime)`. Disable tick when not needed.

```cpp
void ALivingEntity::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    BehaviorComponent->TickState(DeltaTime);
    StatusEffectComponent->TickEffects(DeltaTime);
    ProgressionComponent->TickGrowth(DeltaTime);
}
```

**Optimization**: Disable tick when idle: `SetActorTickEnabled(false)`. Use timers (`FTimerHandle`) instead of tick for periodic logic.

**In Aethelgard**: Existing tick pattern. Extend with explicit `Update Method` for AI behavior, status effect ticking, hunger/thirst decay, and growth system.

---

## 25. Double Buffer

**When**: You need to avoid data tearing when one system writes state while another reads it (multithreading or physics → gameplay).

**UE5 Idiom**: Unreal's physics system uses double buffering internally. For custom double buffering, use two arrays and swap.

```cpp
class FVoxelChunkData
{
public:
    void Write(int32 Index, float Value)
    {
        WriteBuffer[Index] = Value;
    }

    void Swap()
    {
        // Atomic swap of read/write buffers
        Exchange(ReadBuffer, WriteBuffer);
    }

    float Read(int32 Index) const
    {
        return ReadBuffer[Index];
    }

private:
    TArray<float> ReadBuffer;
    TArray<float> WriteBuffer;
};
```

**In Aethelgard**: Useful for voxel chunk mesh generation (physics reads current mesh while background thread computes new mesh). Also for network interpolation state.

---

## 26. Spatial Partition

**When**: You need to efficiently query objects by position (proximity detection, collision, AI perception).

**UE5 Idiom**: UE has built-in spatial queries: `UWorld::OverlapMultiByChannel()`, `UWorld::SweepMultiByChannel()`, `UNavigationSystemV1`. For custom spatial partitioning, use a grid (simplest) or octree.

```cpp
class FSpatialGrid
{
public:
    FSpatialGrid(float InCellSize, const FIntVector& InGridSize)
        : CellSize(InCellSize), GridSize(InGridSize)
    {
        Cells.SetNum(GridSize.X * GridSize.Y * GridSize.Z);
    }

    void Insert(AActor* Actor)
    {
        FIntVector Cell = WorldToCell(Actor->GetActorLocation());
        int32 Index = Cell.X + Cell.Y * GridSize.X + Cell.Z * GridSize.X * GridSize.Y;
        Cells[Index].Add(Actor);
    }

    TArray<AActor*> QueryInRadius(const FVector& Center, float Radius)
    {
        TArray<AActor*> Result;
        FIntVector MinCell = WorldToCell(Center - FVector(Radius));
        FIntVector MaxCell = WorldToCell(Center + FVector(Radius));

        for (int32 Z = MinCell.Z; Z <= MaxCell.Z; Z++)
            for (int32 Y = MinCell.Y; Y <= MaxCell.Y; Y++)
                for (int32 X = MinCell.X; X <= MaxCell.X; X++)
                    Result.Append(GetCell(X, Y, Z));
        return Result;
    }

private:
    float CellSize;
    FIntVector GridSize;
    TArray<TArray<AActor*>> Cells;

    FIntVector WorldToCell(const FVector& WorldPos) const
    {
        return FIntVector(
            FMath::FloorToInt(WorldPos.X / CellSize),
            FMath::FloorToInt(WorldPos.Y / CellSize),
            FMath::FloorToInt(WorldPos.Z / CellSize)
        );
    }

    TArray<AActor*>& GetCell(int32 X, int32 Y, int32 Z)
    {
        return Cells[X + Y * GridSize.X + Z * GridSize.X * GridSize.Y];
    }
};
```

**In Aethelgard**: Essential for voxel world chunk management, AI perception, and physics optimization. UE's built-in spatial queries should be preferred unless custom partitioning is needed.

---

## 27. Event Queue

**When**: Events are produced faster than they can be consumed, or you need to decouple event producers from consumers.

**UE5 Idiom**: Use `DECLARE_DYNAMIC_MULTICAST_DELEGATE` for immediate dispatch. For queued processing, use a `TQueue<>` or `TArray<FQueuedEvent>` processed each tick.

```cpp
USTRUCT()
struct FQueuedEvent
{
    GENERATED_BODY()

    UPROPERTY()
    UObject* Sender;

    UPROPERTY()
    UObject* Target;

    UPROPERTY()
    FGameplayTag EventTag;

    UPROPERTY()
    TMap<FName, float> FloatParams;
};

UCLASS()
class UEventQueueSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void EnqueueEvent(UObject* Sender, UObject* Target, FGameplayTag EventTag)
    {
        FQueuedEvent Event;
        Event.Sender = Sender;
        Event.Target = Target;
        Event.EventTag = EventTag;
        Queue.Enqueue(Event);
    }

    void TickEventQueue(float DeltaTime)
    {
        FQueuedEvent Event;
        while (Queue.Dequeue(Event))
        {
            // Process event with decoupled dispatch
            ProcessEvent(Event);
        }
    }

private:
    TQueue<FQueuedEvent> Queue;

    void ProcessEvent(const FQueuedEvent& Event);
};
```

**In Aethelgard**: Use for decoupling combat events (damage → death → loot → XP), network RPC queuing, and UI event batching.

---

## 28. Service Locator

**When**: You need global access to a service without coupling to its concrete implementation.

**UE5 Idiom**: **Do not implement custom service locators.** Use `UGameInstanceSubsystem`, `UWorldSubsystem`, or `UEngineSubsystem`. They are Unreal's built-in service locators.

```cpp
// Access pattern
USpawnManagerSubsystem* Spawner = GetGameInstance()->GetSubsystem<USpawnManagerSubsystem>();
UWorldTimeSubsystem* TimeSystem = GetWorld()->GetSubsystem<UWorldTimeSubsystem>();
UEventQueueSubsystem* EventSystem = GetWorld()->GetSubsystem<UEventQueueSubsystem>();
```

**Rule**: Always use UE Subsystems. Never write `Get()` singletons or manual service locators.

**In Aethelgard**: `UGameInstanceSubsystem` for persistence, player manager, spawn manager. `UWorldSubsystem` for world time, event queue, AI perception manager.

---

## 29. Component

**When**: You need to compose game objects from reusable, swappable parts rather than deep inheritance hierarchies.

**UE5 Idiom**: **This IS Unreal Engine's architecture.** `UActorComponent` is the canonical Component pattern. Compose actors from components instead of deep class hierarchies.

```cpp
// Prefer composition over inheritance
class ALivingEntity : public AActor
{
    // ~15 components, each handling one concern
    UCombatComponent* Combat;
    UBehaviorComponent* Behavior;
    UAbilityComponent* Abilities;
    UInventoryComponent* Inventory;
    UEquipmentComponent* Equipment;
    UStatusEffectComponent* StatusEffects;
    UProgressionComponent* Progression;
    UClassComponent* ClassData;
    UPerceptionComponent* Perception;
    UThreatComponent* Threat;
    UGrowthComponent* Growth;
    UCraftingComponent* Crafting;
    UBuildComponent* Building;
    URiderComponent* Riding;
};
```

**In Aethelgard**: This is the **foundational pattern** of the entire architecture (see UML line 759–801 and README). All entity behavior is component-based.

---

## 30. Subclass Sandbox

**When**: You need many variants of a behavior (e.g., abilities, items, status effects) defined via data + limited code, without creating hundreds of C++ subclasses.

**UE5 Idiom**: Use `UDataAsset` templates with `TSubclassOf<>` for extensibility. Combine with Strategy pattern.

```cpp
// Sandbox: one ability C++ class, configured by data
UCLASS(BlueprintType)
class UAbilityDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    float Cooldown;

    UPROPERTY(EditAnywhere)
    float ManaCost;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UAbilityEffect> EffectClass;

    UPROPERTY(EditAnywhere)
    FGameplayTagContainer Tags;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> Icon;
};

// 100 abilities can exist as data assets — no new C++ classes needed
```

**In Aethelgard**: `UAbilityDefinition`, `UItemBase`, `URecipe`, `ULootTable`, `UClassDefinition` — all follow Subclass Sandbox. Add new abilities/items/recipes as data assets, not C++ subclasses.

---

## 31. Type Object

**When**: Many "types" (species, item categories, classes) need to exist without each being a separate C++ class.

**UE5 Idiom**: `UDataAsset` or `UObject` instances define types. A single C++ class handles behavior for all types.

```cpp
// No C++ subclass per race — Race is data
UCLASS()
class URaceData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FName RaceName;

    UPROPERTY(EditAnywhere)
    UBodyPlan* BodyPlan;

    UPROPERTY(EditAnywhere)
    TArray<URaceTrait*> Traits;

    UPROPERTY(EditAnywhere)
    UAttributeSetDefinition* Attributes;

    UPROPERTY(EditAnywhere)
    UNeedSetDefinition* Needs;
};

// One C++ class handles all races
class ALivingEntity : public AActor
{
    void ConfigureForRace(URaceData* Race)
    {
        BodyPlan = Race->BodyPlan;
        ApplyAttributes(Race->Attributes);
    }
};
```

**In Aethelgard**: `URaceData`, `UBodyPlan`, `UClassDefinition`, `UItemBase`, `UBiome` — all are Type Objects. This is the core data-driven principle of the project. **Do NOT create C++ subclasses for game content types.**

---

## 32. Bytecode

**When**: You need scriptable behavior that can be modified without recompiling the game (modding, data-driven AI, custom abilities).

**UE5 Idiom**: Behavior Trees (`UBehaviorTree` / `UBTTaskNode` / `UBTDecorator`) are Unreal's Bytecode pattern. For custom scripting, use `FGameplayTag`-based behavior selection or a lightweight VM.

```cpp
// Instead of custom bytecode: use Behavior Trees + Blackboard
// Behavior Tree tasks are C++ nodes that act as bytecode instructions

UCLASS()
class UBTTask_ExecuteAbility : public UBTTaskNode
{
    GENERATED_BODY()

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override
    {
        AAIController* AIController = OwnerComp.GetAIOwner();
        if (ALivingEntity* Entity = Cast<ALivingEntity>(AIController->GetPawn()))
        {
            Entity->Abilities->ActivateBestAbility();
            return EBTNodeResult::Succeeded;
        }
        return EBTNodeResult::Failed;
    }
};
```

**In Aethelgard**: Behavior Trees for AI (already in the stack). No custom bytecode VM needed.

---

## 33. Dirty Flag

**When**: An expensive computation should be deferred until its result is actually needed, with caching and invalidation.

**UE5 Idiom**: `bIsDirty` pattern. UE's transform update is a Dirty Flag system.

```cpp
class FVoxelChunk
{
public:
    void MarkMeshDirty() { bMeshDirty = true; }

    void GenerateMeshIfNeeded()
    {
        if (bMeshDirty)
        {
            RebuildMesh();
            bMeshDirty = false;
        }
    }

    void SetVoxel(int32 X, int32 Y, int32 Z, EMaterialType Material)
    {
        VoxelData[X][Y][Z] = Material;
        MarkMeshDirty(); // Invalidate cached mesh
    }

private:
    bool bMeshDirty = true;
    TArray<TArray<TArray<EMaterialType>>> VoxelData;

    void RebuildMesh()
    {
        // Expensive mesh generation
    }
};
```

**In Aethelgard**: Use for voxel chunk mesh regeneration (rebuild only when dirtied), pathfinding grid recalculation, UI stats recalculation. Any deferred computation benefits from this.

---

## 34. Object Pool

**When**: Objects are frequently created and destroyed (projectiles, particles, status effect instances, pickups). Allocation overhead is measurable.

**UE5 Idiom**: Use `TQueue<AActor*>`, `UWorld::SpawnActorDeferred` with a pool. Unreal's `UPool` object pattern.

```cpp
UCLASS()
class AActorPool : public AActor
{
    GENERATED_BODY()

public:
    void Initialize(TSubclassOf<AActor> InClass, int32 PoolSize)
    {
        PoolClass = InClass;
        for (int32 i = 0; i < PoolSize; i++)
        {
            AActor* Instance = GetWorld()->SpawnActorDeferred<AActor>(PoolClass, FTransform::Identity);
            Instance->SetActorHiddenInGame(true);
            Instance->SetActorEnableCollision(false);
            Instance->FinishSpawning(FTransform::Identity);
            Pool.Enqueue(Instance);
        }
    }

    AActor* Acquire()
    {
        AActor* Instance;
        if (Pool.Dequeue(Instance))
        {
            Instance->SetActorHiddenInGame(false);
            Instance->SetActorEnableCollision(true);
            return Instance;
        }
        return nullptr; // Pool exhausted — optionally expand
    }

    void Release(AActor* Instance)
    {
        Instance->SetActorHiddenInGame(true);
        Instance->SetActorEnableCollision(false);
        Pool.Enqueue(Instance);
    }

private:
    TQueue<AActor*> Pool;
    UPROPERTY()
    TSubclassOf<AActor> PoolClass;
};
```

**In Aethelgard**: Use for projectiles, particle effects, damage numbers, loot pickups, status effect visualizations. Any high-frequency spawn/destroy cycle.

---

## 35. Data Locality

**When**: Performance-critical code accesses many objects sequentially, and cache misses are a bottleneck.

**UE5 Idiom**: Prefer `TArray` of structs (SoA) over arrays of pointers (AoS). Use contiguous memory for hot paths.

```cpp
// Before (AoS — pointer chasing, cache misses):
TArray<UParticleSystemComponent*> ParticleComponents;

// After (SoA — contiguous, cache-friendly):
struct FParticleInstance
{
    FVector Position;
    FVector Velocity;
    float Lifetime;
    float Age;
    EParticleType Type;
};

TArray<FParticleInstance> ParticleData; // Contiguous in memory
```

**In Aethelgard**: Apply to voxel chunk data, particle systems, projectile updates, AI perception results. Profile first — only optimize when profiling shows cache misses are a bottleneck.

---

# Part 5: Architectural Principles

## Data-Driven Design

**Principle**: All game balance data lives in `UDataAsset` / `UPrimaryDataAsset`. Behavior is in C++. Configuration is in assets.

- `URaceData` defines race stats, needs, body plan
- `UAbilityDefinition` defines ability parameters, effects, cooldowns
- `UItemBase` defines item properties, stacking, crafting uses
- `URecipe` defines crafting inputs and outputs
- `ULootTable` defines drop rates and conditions

**Never hardcode values** that can be data-driven.

## Prefer Unreal Native Solutions

| Pattern | Unreal Native Alternative |
|---|---|
| Service Locator | `UGameInstanceSubsystem` / `UWorldSubsystem` |
| Singleton | `UGameMode`, `UGameState`, `UGameInstance` |
| Observer | `DECLARE_DYNAMIC_MULTICAST_DELEGATE` / `DECLARE_MULTICAST_DELEGATE` |
| Component | `UActorComponent` / `USceneComponent` / `UPrimitiveComponent` |
| Command | Enhanced Input Actions / `UAbilityInstance` |
| State | `UBehaviorTree` / `UAnimInstance` |
| Bytecode | `UBehaviorTree` / `UBTTaskNode` |
| Iterator | `TIterator<>`, `CreateIterator()` |
| Proxy | `TSoftObjectPtr<>` / `TWeakObjectPtr<>` / `TLazyObjectPtr<>` |
| Factory | `NewObject<>()` / `SpawnActorDeferred<>()` |
| Prototype | `DuplicateObject<>()` / `UDataAsset` inheritance |

---

# Pattern Selection Guide

| Problem | Priority 1 (Game/GoF Pattern) | Priority 2 (UE Idiom) |
|---|---|---|
| Entity creation | Factory Method | `SpawnActorDeferred<>()` |
| Object reuse | Object Pool | `TQueue<AActor*>` pool |
| State-dependent behavior | State | Enum + switch / State Machine |
| Swappable algorithms | Strategy | Interface + `TUniquePtr<>` |
| Event decoupling | Observer / Event Queue | `DECLARE_MULTICAST_DELEGATE` |
| Data variants | Type Object / Prototype | `UPrimaryDataAsset` inheritance |
| Per-frame updates | Update Method | `AActor::Tick()` |
| Spatial queries | Spatial Partition | UE built-in overlap/sweep |
| Performance (many entities) | Data Locality + Flyweight | `TArray<F>` struct-of-arrays |
| Persistence / undo | Memento | `USaveGame` |
| Centralized complex flows | Mediator | `UGameInstanceSubsystem` |
| Composable behavior | Component | `UActorComponent` |
| Asset lazy loading | Proxy | `TSoftObjectPtr<>` |
| Deferred computation | Dirty Flag | `bIsDirty` pattern |
| Scriptable behavior | Bytecode | `UBehaviorTree` |
| Lazy initialization | Proxy / Virtual Proxy | `TSoftObjectPtr<>` |

---

# Anti-Patterns to Avoid

| Anti-Pattern | Why | Instead Use |
|---|---|---|
| **God Class** | One class knows/does everything | **Component pattern** — split concerns |
| **Manual Singleton** | Hard to test, couples globally | `UGameInstanceSubsystem` / `UWorldSubsystem` |
| **Circular Dependencies** | Components referencing each other directly | **Mediator** or **Event Queue** |
| **Spaghetti Events** | Unbounded delegate chains | **Event Queue** with structured processing |
| **Over-abstracted** | Too many interfaces for simple logic | **YAGNI** — only abstract when needed |
| **Premature Optimization** | Complex patterns for non-bottleneck code | **Profile first**, then optimize |
| **Fighting the Engine** | Custom implementation where UE provides a native solution | **Use Unreal's tools** — they're battle-tested |
| **Blueprint-Only Logic** | Logic in BP graphs, hard to refactor | **C++ Only** (see cpp-only skill) |
| **Deep Inheritance** | 10+ levels of class hierarchy | **Component composition** + Type Object |
