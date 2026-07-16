// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/BuildComponent.h"
#include "Terrain/VoxelWorld.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

UBuildComponent::UBuildComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsDestroying)
    {
        TickDestroy(DeltaTime);
    }
}

bool UBuildComponent::TraceBlock(FIntVector& OutBlock, FIntVector& OutPlacePos) const
{
    APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
    if (!PC)
    {
        AActor* Owner = GetOwner();
        PC = Owner ? Owner->GetWorld()->GetFirstPlayerController() : nullptr;
    }
    if (!PC) return false;

    FVector CameraLoc;
    FRotator CameraRot;
    PC->GetPlayerViewPoint(CameraLoc, CameraRot);

    FVector End = CameraLoc + CameraRot.Vector() * ReachDistance;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    if (GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, End, ECC_Visibility, Params))
    {
        FVector HitPoint = Hit.Location;
        FVector Normal = Hit.Normal;

        FIntVector HitBlock(
            FMath::FloorToInt(HitPoint.X),
            FMath::FloorToInt(HitPoint.Y),
            FMath::FloorToInt(HitPoint.Z)
        );

        if (HitPoint.X - (float)HitBlock.X > 0.999f) HitBlock.X++;
        if (HitPoint.Y - (float)HitBlock.Y > 0.999f) HitBlock.Y++;
        if (HitPoint.Z - (float)HitBlock.Z > 0.999f) HitBlock.Z++;

        FIntVector PlaceBlock = HitBlock;
        if (Normal.X > 0.5f)      PlaceBlock.X += 1;
        else if (Normal.X < -0.5f) PlaceBlock.X -= 1;
        else if (Normal.Y > 0.5f)  PlaceBlock.Y += 1;
        else if (Normal.Y < -0.5f) PlaceBlock.Y -= 1;
        else if (Normal.Z > 0.5f)  PlaceBlock.Z += 1;
        else if (Normal.Z < -0.5f) PlaceBlock.Z -= 1;

        OutBlock = HitBlock;
        OutPlacePos = PlaceBlock;
        return true;
    }

    return false;
}

bool UBuildComponent::GetTargetBlock(FIntVector& OutBlock, FIntVector& OutPlacePos) const
{
    return TraceBlock(OutBlock, OutPlacePos);
}

void UBuildComponent::StartDestroy()
{
    FIntVector BlockPos, PlacePos;
    if (!TraceBlock(BlockPos, PlacePos))
        return;

    if (!TargetWorld)
        return;

    bIsDestroying = true;
    CurrentDestroyProgress = 0.0f;
    CurrentTargetBlock = BlockPos;
    CurrentPlacePos = PlacePos;
    OnTargetBlockChanged.Broadcast(CurrentTargetBlock);

    SetComponentTickEnabled(true);
}

void UBuildComponent::TickDestroy(float DeltaTime)
{
    if (!bIsDestroying || !TargetWorld)
    {
        CancelDestroy();
        return;
    }

    FIntVector CurrentBlock, PlacePos;
    if (!TraceBlock(CurrentBlock, PlacePos) || CurrentBlock != CurrentTargetBlock)
    {
        CancelDestroy();
        return;
    }

    CurrentDestroyProgress += DeltaTime / DestroyTime;
    OnDestroyProgress.Broadcast(CurrentDestroyProgress);

    if (CurrentDestroyProgress >= 1.0f)
    {
        EBlockId Block = TargetWorld->GetBlock(CurrentTargetBlock.X, CurrentTargetBlock.Y, CurrentTargetBlock.Z);
        if (Block != EBlockId::Air && Block != EBlockId::Water && Inventory)
        {
            UItemBlock* Item = UItemBlock::Create(Block, this);
            if (Item)
            {
                Inventory->AddItem(Item, 1);
            }
        }

        TargetWorld->SetBlock(CurrentTargetBlock.X, CurrentTargetBlock.Y, CurrentTargetBlock.Z, EBlockId::Air);

        bIsDestroying = false;
        CurrentDestroyProgress = 0.0f;
        SetComponentTickEnabled(false);
    }
}

void UBuildComponent::CancelDestroy()
{
    bIsDestroying = false;
    CurrentDestroyProgress = 0.0f;
    SetComponentTickEnabled(false);
    OnDestroyProgress.Broadcast(0.0f);
}

void UBuildComponent::PlaceBlock()
{
    if (!TargetWorld || !Inventory)
        return;

    if (!Inventory->HasItem(SelectedBlock, 1))
        return;

    FIntVector BlockPos, PlacePos;
    if (!TraceBlock(BlockPos, PlacePos))
        return;

    EBlockId ExistingBlock = TargetWorld->GetBlock(PlacePos.X, PlacePos.Y, PlacePos.Z);
    if (ExistingBlock != EBlockId::Air && ExistingBlock != EBlockId::Water)
        return;

    TargetWorld->SetBlock(PlacePos.X, PlacePos.Y, PlacePos.Z, SelectedBlock);
    Inventory->RemoveItem(SelectedBlock, 1);
}
