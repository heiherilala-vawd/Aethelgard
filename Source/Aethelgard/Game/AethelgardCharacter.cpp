// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AethelgardCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AethelgardInteraction/BuildComponent.h"
#include "AethelgardInteraction/InventoryComponent.h"
#include "AethelgardTerrain/VoxelWorld.h"
#include "AethelgardTerrain/WorldGeneratorComponent.h"
#include "Game/AethelgardGameMode.h"

AAethelgardCharacter::AAethelgardCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArmComponent->SetupAttachment(RootComponent);
    SpringArmComponent->TargetArmLength = 600.0f;
    SpringArmComponent->bUsePawnControlRotation = true;
    SpringArmComponent->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(SpringArmComponent);

    BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));
    Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

    GetCharacterMovement()->MaxWalkSpeed = 800.0f;
    GetCharacterMovement()->JumpZVelocity = 420.0f;
    GetCharacterMovement()->AirControl = 0.2f;
    GetCharacterMovement()->GravityScale = 1.0f;
}

void AAethelgardCharacter::BeginPlay()
{
    Super::BeginPlay();

    AAethelgardGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AAethelgardGameMode>() : nullptr;
    if (GM)
    {
        AVoxelWorld* VoxelWorld = GM->GetVoxelWorld();
        BuildComponent->SetVoxelWorld(VoxelWorld);
        BuildComponent->SetInventory(Inventory);
    }

    Inventory->InitializeDefaultInventory();

    if (GM)
    {
        AVoxelWorld* VoxelWorld = GM->GetVoxelWorld();
        if (VoxelWorld && VoxelWorld->GetWorldGenerator())
        {
            float BS = VoxelWorld->BlockScale;
            FVector Loc = GetActorLocation();
            float TerrainH = VoxelWorld->GetWorldGenerator()->GetHeight(
                FMath::FloorToInt(Loc.X / BS),
                FMath::FloorToInt(Loc.Y / BS)
            );
            Loc.Z = TerrainH * BS + 2.0f * BS;
            SetActorLocation(Loc);
        }
    }
}

void AAethelgardCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AAethelgardCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AAethelgardCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &AAethelgardCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AAethelgardCharacter::LookUp);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Destroy", IE_Pressed, this, &AAethelgardCharacter::OnStartDestroy);
    PlayerInputComponent->BindAction("Destroy", IE_Released, this, &AAethelgardCharacter::OnReleaseDestroy);
    PlayerInputComponent->BindAction("Place", IE_Pressed, this, &AAethelgardCharacter::OnPlaceBlock);
    PlayerInputComponent->BindAction("NextBlock", IE_Pressed, this, &AAethelgardCharacter::OnNextBlock);
    PlayerInputComponent->BindAction("PrevBlock", IE_Pressed, this, &AAethelgardCharacter::OnPrevBlock);
}

void AAethelgardCharacter::MoveForward(float Value)
{
    if (Value != 0.0f)
        AddMovementInput(GetActorForwardVector(), Value);
}

void AAethelgardCharacter::MoveRight(float Value)
{
    if (Value != 0.0f)
        AddMovementInput(GetActorRightVector(), Value);
}

void AAethelgardCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AAethelgardCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AAethelgardCharacter::OnStartDestroy()
{
    BuildComponent->StartDestroy();
}

void AAethelgardCharacter::OnReleaseDestroy()
{
    BuildComponent->CancelDestroy();
}

void AAethelgardCharacter::OnPlaceBlock()
{
    BuildComponent->PlaceBlock();
}

void AAethelgardCharacter::OnNextBlock()
{
    uint8 Current = static_cast<uint8>(BuildComponent->SelectedBlock);
    Current++;
    if (Current >= static_cast<uint8>(EBlockId::MAX))
        Current = 1;
    BuildComponent->SelectedBlock = static_cast<EBlockId>(Current);
}

void AAethelgardCharacter::OnPrevBlock()
{
    uint8 Current = static_cast<uint8>(BuildComponent->SelectedBlock);
    if (Current <= 1)
        Current = static_cast<uint8>(EBlockId::MAX) - 1;
    else
        Current--;
    BuildComponent->SelectedBlock = static_cast<EBlockId>(Current);
}
