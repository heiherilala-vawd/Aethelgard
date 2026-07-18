// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AethelgardInteraction/ItemBlock.h"
#include "AethelgardTerrain/SaveSystem.h"
#include "InventoryComponent.generated.h"

USTRUCT()
struct AETHELGARDINTERACTION_API FItemStack
{
    GENERATED_BODY()

    UPROPERTY()
    UItemBlock* Item = nullptr;

    UPROPERTY()
    int32 Quantity = 0;
};

UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class AETHELGARDINTERACTION_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    UPROPERTY(EditAnywhere, Category = "Inventory")
    int32 MaxSlots = 36;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(UItemBlock* Item, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(EBlockId BlockId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool HasItem(EBlockId BlockId, int32 Quantity) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetItemCount(EBlockId BlockId) const;

    const TArray<FItemStack>& GetSlots() const { return Slots; }

    void InitializeDefaultInventory();
    TArray<FInventorySlotSaveData> GetSaveData() const;
    void LoadFromSaveData(const TArray<FInventorySlotSaveData>& Data);

private:
    UPROPERTY()
    TArray<FItemStack> Slots;

    int32 FindSlot(EBlockId BlockId) const;
    int32 FindOrCreateSlot(EBlockId BlockId);
};
