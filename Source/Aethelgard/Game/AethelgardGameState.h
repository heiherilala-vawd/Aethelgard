// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "AethelgardGameState.generated.h"

UCLASS()
class AAethelgardGameState : public AGameState
{
    GENERATED_BODY()

public:
    UPROPERTY(ReplicatedUsing = OnRep_WorldSeed, BlueprintReadOnly, Category = "World")
    int32 WorldSeed = 0;

    UFUNCTION()
    void OnRep_WorldSeed();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
