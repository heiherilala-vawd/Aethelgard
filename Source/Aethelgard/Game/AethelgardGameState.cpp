// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardGameState.h"
#include "Net/UnrealNetwork.h"

void AAethelgardGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAethelgardGameState, WorldSeed);
}

void AAethelgardGameState::OnRep_WorldSeed()
{
}
