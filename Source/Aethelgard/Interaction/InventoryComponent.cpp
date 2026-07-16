// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Slots.SetNum(MaxSlots);
}

bool UInventoryComponent::AddItem(UItemBlock* Item, int32 Quantity)
{
    if (!Item || Quantity <= 0)
        return false;

    int32 SlotIndex = FindSlot(Item->BlockId);
    if (SlotIndex < 0)
        SlotIndex = FindOrCreateSlot(Item->BlockId);

    if (SlotIndex < 0)
        return false;

    Slots[SlotIndex].Item = Item;
    Slots[SlotIndex].Quantity += Quantity;

    if (Slots[SlotIndex].Quantity > Item->MaxStack)
    {
        int32 Overflow = Slots[SlotIndex].Quantity - Item->MaxStack;
        Slots[SlotIndex].Quantity = Item->MaxStack;

        int32 NewSlot = FindOrCreateSlot(Item->BlockId);
        if (NewSlot >= 0)
        {
            Slots[NewSlot].Item = Item;
            Slots[NewSlot].Quantity += Overflow;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool UInventoryComponent::RemoveItem(EBlockId BlockId, int32 Quantity)
{
    int32 SlotIndex = FindSlot(BlockId);
    if (SlotIndex < 0 || Slots[SlotIndex].Quantity < Quantity)
        return false;

    Slots[SlotIndex].Quantity -= Quantity;
    if (Slots[SlotIndex].Quantity <= 0)
    {
        Slots[SlotIndex].Item = nullptr;
        Slots[SlotIndex].Quantity = 0;
    }

    return true;
}

bool UInventoryComponent::HasItem(EBlockId BlockId, int32 Quantity) const
{
    return GetItemCount(BlockId) >= Quantity;
}

int32 UInventoryComponent::GetItemCount(EBlockId BlockId) const
{
    int32 Total = 0;
    for (const FItemStack& Stack : Slots)
    {
        if (Stack.Item && Stack.Item->BlockId == BlockId)
        {
            Total += Stack.Quantity;
        }
    }
    return Total;
}

void UInventoryComponent::InitializeDefaultInventory()
{
    UItemBlock* DirtItem = UItemBlock::Create(EBlockId::Dirt, this);
    UItemBlock* StoneItem = UItemBlock::Create(EBlockId::Stone, this);
    UItemBlock* WoodItem = UItemBlock::Create(EBlockId::Wood, this);
    UItemBlock* GrassItem = UItemBlock::Create(EBlockId::Grass, this);
    UItemBlock* SandItem = UItemBlock::Create(EBlockId::Sand, this);

    AddItem(DirtItem, 32);
    AddItem(StoneItem, 32);
    AddItem(WoodItem, 16);
    AddItem(GrassItem, 32);
    AddItem(SandItem, 32);
}

int32 UInventoryComponent::FindSlot(EBlockId BlockId) const
{
    for (int32 i = 0; i < Slots.Num(); i++)
    {
        if (Slots[i].Item && Slots[i].Item->BlockId == BlockId && Slots[i].Quantity < Slots[i].Item->MaxStack)
            return i;
    }
    for (int32 i = 0; i < Slots.Num(); i++)
    {
        if (Slots[i].Item && Slots[i].Item->BlockId == BlockId)
            return i;
    }
    return -1;
}

int32 UInventoryComponent::FindOrCreateSlot(EBlockId BlockId)
{
    int32 Existing = FindSlot(BlockId);
    if (Existing >= 0)
        return Existing;

    for (int32 i = 0; i < Slots.Num(); i++)
    {
        if (Slots[i].Item == nullptr || Slots[i].Quantity <= 0)
            return i;
    }

    return -1;
}
