// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include <Camera/CameraComponent.h>
#include <Components/InputComponent.h>
#include <Components/SceneComponent.h>
#include <Components/StaticMeshComponent.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>
#include <Public/TimerManager.h>
#include <Components/CapsuleComponent.h>
#include <NavigationSystem.h>
#include <Components/PostProcessComponent.h>
#include <Materials/MaterialInstanceDynamic.h>

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

void AVRCharacter::OnTeleport()
{
	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::FadeOutAndTeleport, FadeDuration);
}

void AVRCharacter::FadeOutAndTeleport()
{
	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeDuration, FLinearColor::Black);
	FVector destination = DestinationMarker->GetComponentLocation();
	destination.Y += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(destination);
}

bool AVRCharacter::FindTeleportLocation(FVector& outLocation)
{
	FHitResult hitResult;
	FVector start = Camera->GetComponentLocation();
	FVector end = start + Camera->GetForwardVector() * MaxTeleportDistance;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		hitResult,
		Camera->GetComponentLocation(),
		end,
		ECollisionChannel::ECC_Visibility
	);

	if (!bHit) return false;

	UNavigationSystemV1* nav = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	FNavLocation navLoc;
	bool bProjected = nav->ProjectPointToNavigation((FVector)(hitResult.Location), navLoc, FVector::OneVector * 100.0f); 

	if (!bProjected) return false;

	outLocation = navLoc.Location;
	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector newLoc;
	bool foundLoc = FindTeleportLocation(newLoc);
	if (foundLoc) {
		DestinationMarker->SetWorldLocation(newLoc);
		DestinationMarker->SetVisibility(true);
	}
	else {
		DestinationMarker->SetVisibility(false);
	}
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

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Destination Marker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());

}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	BlinkerMaterialDynamic = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
	PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialDynamic);
	BlinkerMaterialDynamic->SetScalarParameterValue(TEXT("Radius"), 1.0f);

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Playspace Collision detection
	FVector CamDelta = Camera->GetComponentLocation() - GetActorLocation();
	CamDelta.Z = 0;
	SetActorLocation(GetActorLocation() + CamDelta);
	VRRoot->SetWorldLocation(VRRoot->GetComponentLocation() - CamDelta);

	UpdateBlinkers();
	UpdateDestinationMarker();
}

void AVRCharacter::UpdateBlinkers()
{
	float speed = GetVelocity().Size();
	float radius = RadiusVsSpeed->GetFloatValue(speed);
	BlinkerMaterialDynamic->SetScalarParameterValue(TEXT("Radius"), radius);

	FVector2D center = GetBlinkerCenter();
	BlinkerMaterialDynamic->SetVectorParameterValue(TEXT("Center"), FLinearColor(center.X, center.Y, 0));
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5f, 0.5f);
	}
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) < 0) 
	{
		MovementDirection *= -1.0f;
	}
	FVector WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 100.0f;

	APlayerController* Controller = Cast<APlayerController>(GetController());

	if (!Controller)
	{
		return FVector2D(0.5f, 0.5f);
	}

	FVector2D ScreenLocation;
	Controller->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenLocation);

	int SizeX, SizeY;
	Controller->GetViewportSize(SizeX, SizeY);

	FVector2D Center(ScreenLocation.X / SizeX, ScreenLocation.Y / SizeY);

	return Center;
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Horizontal"), this, &AVRCharacter::OnHorizontal);
	PlayerInputComponent->BindAxis(TEXT("Vertical"), this, &AVRCharacter::OnVertical);
	PlayerInputComponent->BindAction(TEXT("Teleport"), EInputEvent::IE_Pressed, this, &AVRCharacter::OnTeleport);
}

