// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include <MotionControllerComponent.h>
#include <Kismet/GameplayStatics.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>

// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Motion Controller"));
	MotionController->bDisplayDeviceModel = true;
	SetRootComponent(MotionController);
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsClimbing)
	{
		FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;
		AActor* Parent = GetAttachParentActor();
		Parent->AddActorWorldOffset(-HandControllerDelta);
	}
}

UMotionControllerComponent* AHandController::GetMotionController()
{
	return MotionController;
}

void AHandController::SetHand(EControllerHand type)
{
	MotionController->SetTrackingSource(type);
}

void AHandController::PairController(AHandController* Other)
{
	OtherController = Other;
	OtherController->OtherController = this;
}

void AHandController::Grip()
{
	if (!bCanClimb) return;

	OtherController->Release();
	ACharacter* Player = Cast<ACharacter>(GetAttachParentActor());
	Player->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingStartLocation = GetActorLocation();
	bIsClimbing = true;
}

void AHandController::Release()
{
	if (bIsClimbing)
	{
		ACharacter* Player = Cast<ACharacter>(GetAttachParentActor());
		Player->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		bIsClimbing = false;
	}
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool prevCanClimb = bCanClimb;
	UpdateClimbState();
	if (!prevCanClimb && bCanClimb)
	{
		APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
	}
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	UpdateClimbState();
}

void AHandController::UpdateClimbState()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Climbable")))
		{
			bCanClimb = true;
			return;
		}
	}
	bCanClimb = false;
}

