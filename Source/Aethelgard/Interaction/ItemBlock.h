// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Terrain/BlockRegistry.h"
#include "ItemBlock.generated.h"

UCLASS()
class UItemBlock : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Item")
    EBlockId BlockId = EBlockId::Stone;

    UPROPERTY(EditAnywhere, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, Category = "Item")
    int32 MaxStack = 99;

    UPROPERTY(EditAnywhere, Category = "Item")
    FColor TintColor = FColor::White;

    static UItemBlock* Create(EBlockId InBlockId, UObject* Outer)
    {
        UItemBlock* Item = NewObject<UItemBlock>(Outer);
        Item->BlockId = InBlockId;

        switch (InBlockId)
        {
        case EBlockId::Stone:  Item->DisplayName = FText::FromString(TEXT("Stone"));  Item->TintColor = FColor(128,128,128); break;
        case EBlockId::Dirt:   Item->DisplayName = FText::FromString(TEXT("Dirt"));   Item->TintColor = FColor(139,90,43); break;
        case EBlockId::Grass:  Item->DisplayName = FText::FromString(TEXT("Grass"));  Item->TintColor = FColor(34,139,34); break;
        case EBlockId::Sand:   Item->DisplayName = FText::FromString(TEXT("Sand"));   Item->TintColor = FColor(194,178,128); break;
        case EBlockId::Wood:   Item->DisplayName = FText::FromString(TEXT("Wood"));   Item->TintColor = FColor(101,67,33); break;
        default:               Item->DisplayName = FText::FromString(TEXT("Block"));  break;
        }

        return Item;
    }
};
