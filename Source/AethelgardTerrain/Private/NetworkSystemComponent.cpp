// Copyright Epic Games, Inc. All Rights Reserved.

#include "AethelgardTerrain/NetworkSystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UNetworkSystemComponent::UNetworkSystemComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

bool UNetworkSystemComponent::HasAuthority() const
{
    AActor* Owner = GetOwner();
    return Owner && Owner->HasAuthority();
}

void UNetworkSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNetworkSystemComponent, WorldSeed);
}

bool UNetworkSystemComponent::ServerRequestModifyBlock_Validate(int32 X, int32 Y, int32 Z, uint8 NewBlockId)
{
    return true;
}

void UNetworkSystemComponent::ServerRequestModifyBlock_Implementation(int32 X, int32 Y, int32 Z, uint8 NewBlockId)
{
    if (!HasAuthority())
        return;

    MulticastBlockModified(X, Y, Z, NewBlockId);
}

bool UNetworkSystemComponent::ServerRequestPlaceBlock_Validate(int32 X, int32 Y, int32 Z, uint8 BlockId)
{
    return BlockId > 0 && BlockId < static_cast<uint8>(EBlockId::MAX);
}

void UNetworkSystemComponent::ServerRequestPlaceBlock_Implementation(int32 X, int32 Y, int32 Z, uint8 BlockId)
{
    if (!HasAuthority())
        return;

    MulticastBlockModified(X, Y, Z, BlockId);
}

void UNetworkSystemComponent::MulticastBlockModified_Implementation(int32 X, int32 Y, int32 Z, uint8 NewBlockId)
{
    OnBlockChangeReceived.Broadcast(X, Y, Z, NewBlockId);
}

void UNetworkSystemComponent::MulticastWorldSeed_Implementation(int32 InSeed)
{
    WorldSeed = InSeed;
    OnSeedReceived.Broadcast(InSeed);
}
