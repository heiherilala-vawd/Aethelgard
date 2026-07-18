// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AethelgardTerrain/BlockRegistry.h"
#include "NetworkSystemComponent.generated.h"

UCLASS(ClassGroup = (Terrain), meta = (BlueprintSpawnableComponent))
class AETHELGARDTERRAIN_API UNetworkSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNetworkSystemComponent();

    void SetWorldSeed(int32 InSeed) { WorldSeed = InSeed; }
    int32 GetWorldSeed() const { return WorldSeed; }

    bool HasAuthority() const;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestModifyBlock(int32 X, int32 Y, int32 Z, uint8 NewBlockId);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastBlockModified(int32 X, int32 Y, int32 Z, uint8 NewBlockId);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestPlaceBlock(int32 X, int32 Y, int32 Z, uint8 BlockId);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastWorldSeed(int32 InSeed);

    DECLARE_MULTICAST_DELEGATE_FourParams(FOnBlockChangeReceived, int32, int32, int32, uint8);
    FOnBlockChangeReceived OnBlockChangeReceived;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnSeedReceived, int32);
    FOnSeedReceived OnSeedReceived;

private:
    UPROPERTY(Replicated)
    int32 WorldSeed = 0;

    bool ServerRequestModifyBlock_Validate(int32 X, int32 Y, int32 Z, uint8 NewBlockId);
    void ServerRequestModifyBlock_Implementation(int32 X, int32 Y, int32 Z, uint8 NewBlockId);

    bool ServerRequestPlaceBlock_Validate(int32 X, int32 Y, int32 Z, uint8 BlockId);
    void ServerRequestPlaceBlock_Implementation(int32 X, int32 Y, int32 Z, uint8 BlockId);

    void MulticastBlockModified_Implementation(int32 X, int32 Y, int32 Z, uint8 NewBlockId);
    void MulticastWorldSeed_Implementation(int32 InSeed);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
