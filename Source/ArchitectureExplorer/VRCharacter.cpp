// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include <Camera/CameraComponent.h>
#include <Components/InputComponent.h>
#include <Components/SceneComponent.h>

void AVRCharacter::OnHorizontal(float value)
{
	FVector right = Camera->GetRightVector();
	right.Z = 0;
	right.Normalize();
	AddMovementInput(right * value);
}

void AVRCharacter::OnVertical(float value)
{
	FVector fwd = Camera->GetForwardVector();
	fwd.Z = 0;
	fwd.Normalize();
	AddMovementInput(fwd * value);
}

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector CamDelta = Camera->GetComponentLocation() - GetActorLocation();
	CamDelta.Z = 0;
	SetActorLocation(GetActorLocation() + CamDelta);
	VRRoot->SetWorldLocation(VRRoot->GetComponentLocation() - CamDelta);
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Horizontal"), this, &AVRCharacter::OnHorizontal);
	PlayerInputComponent->BindAxis(TEXT("Vertical"), this, &AVRCharacter::OnVertical);

}

