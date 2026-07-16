// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AethelgardCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBuildComponent;
class UInventoryComponent;
class AVoxelWorld;

UCLASS()
class AAethelgardCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAethelgardCharacter();

    UBuildComponent* GetBuildComponent() const { return BuildComponent; }
    UInventoryComponent* GetInventory() const { return Inventory; }

    virtual void BeginPlay() override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* SpringArmComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    UBuildComponent* BuildComponent;

    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    UInventoryComponent* Inventory;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void OnStartDestroy();
    void OnReleaseDestroy();
    void OnPlaceBlock();
    void OnNextBlock();
    void OnPrevBlock();
};
