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
#include <MotionControllerComponent.h>
#include <Kismet/GameplayStaticsTypes.h>
#include <Kismet/GameplayStatics.h>
#include <Components/SplineComponent.h>
#include <Components/SplineMeshComponent.h>

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
	if (!HasDestination) return;

	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::FadeOutAndTeleport, FadeDuration);
}

void AVRCharacter::OnSnapRight()
{
	CurrentSnapTurnDegrees = abs(SnapTurnDegrees);
	SnapTurn();
}

void AVRCharacter::OnSnapLeft()
{
	CurrentSnapTurnDegrees = -abs(SnapTurnDegrees);
	SnapTurn();
}

void AVRCharacter::SnapTurn()
{
	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::SnapTurnEnd, FadeDuration);
}

void AVRCharacter::SnapTurnEnd()
{
	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeDuration, FLinearColor::Black);
	AddControllerYawInput(CurrentSnapTurnDegrees);
}

void AVRCharacter::FadeOutAndTeleport()
{
	APlayerController* controller = Cast<APlayerController>(GetController());
	controller->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeDuration, FLinearColor::Black);
	FVector destination = DestinationMarker->GetComponentLocation();
	destination.Y += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(destination);
}

bool AVRCharacter::FindTeleportLocation(TArray<FVector>& outPath, FVector& outLocation)
{
	FVector look = RightController->GetActorForwardVector();
	look.RotateAngleAxis(-30.0f, RightController->GetActorRightVector());

	// Get the parabolic arc
	FPredictProjectilePathParams ProjectileParams(
		3.0f,
		RightController->GetMotionController()->GetComponentLocation(),
		look * MaxTeleportDistance,
		TeleportDuration,
		ECollisionChannel::ECC_Visibility,
		this
	);

	FPredictProjectilePathResult PathResult;
	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), ProjectileParams, PathResult);

	if (!bHit) return false;

	for (int i = 0; i < PathResult.PathData.Num(); ++i) {
		outPath.Add(PathResult.PathData[i].Location);
	}
	// Project that arc onto the navmesh
	UNavigationSystemV1* nav = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	FNavLocation navLoc;
	bool bProjected = nav->ProjectPointToNavigation((FVector)(PathResult.HitResult.Location), navLoc, FVector::OneVector * 100.0f); 

	if (!bProjected) return false;

	outLocation = navLoc.Location;
	return true;
}

void AVRCharacter::UpdateSpline(const TArray<FVector>& worldPoints)
{
	TeleportPath->ClearSplinePoints(false);
	for (int i = 0; i < worldPoints.Num(); ++i) {
		FVector localLocation = TeleportPath->GetComponentTransform().InverseTransformPosition(worldPoints[i]);
		FSplinePoint point(i, localLocation, ESplinePointType::Curve);
		TeleportPath->AddPoint(point, false);
	}
	TeleportPath->UpdateSpline();

	// Create the laser pointer mesh
	for (int i = 0; i < worldPoints.Num() - 1; ++i) {
		USplineMeshComponent* SplineMesh;
		if (i < TeleportPathMeshPool.Num())
		{
			SplineMesh = TeleportPathMeshPool[i];
		}
		else
		{
			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			TeleportPathMeshPool.Add(SplineMesh);
		}
		SplineMesh->SetVisibility(true);
		SplineMesh->SetRelativeLocation(FVector(0, 0, 0));

		FVector StartLocation, StartTangent;
		FVector EndLocation, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartLocation, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndLocation, EndTangent);
		SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);
	}

	// Hide any unused mesh components
	int hideStart = worldPoints.Num() > 0 ? worldPoints.Num() - 1 : 0;
	for (int i = hideStart; i < TeleportPathMeshPool.Num(); ++i) {
		TeleportPathMeshPool[i]->SetVisibility(false);
	}
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector newLoc;
	TArray<FVector> path;
	HasDestination = FindTeleportLocation(path, newLoc);
	if (HasDestination) {
		DestinationMarker->SetWorldLocation(newLoc);
		DestinationMarker->SetVisibility(true);
		UpdateSpline(path);
	}
	else {
		DestinationMarker->SetVisibility(false);
		TArray<FVector> empty;
		UpdateSpline(empty);
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

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("Teleport Path"));
	TeleportPath->SetupAttachment(VRRoot);

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
	
	LeftController = GetWorld()->SpawnActor<AHandController>(HandController);
	LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	LeftController->SetHand(EControllerHand::Left);
	LeftController->SetOwner(this);

	RightController = GetWorld()->SpawnActor<AHandController>(HandController);
	RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	RightController->SetHand(EControllerHand::Right);
	RightController->SetOwner(this);

	LeftController->PairController(RightController);
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
	PlayerInputComponent->BindAction(TEXT("GripLeft"), EInputEvent::IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), EInputEvent::IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(TEXT("GripRight"), EInputEvent::IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), EInputEvent::IE_Released, this, &AVRCharacter::ReleaseRight);
	PlayerInputComponent->BindAction(TEXT("SnapRight"), EInputEvent::IE_Pressed, this, &AVRCharacter::OnSnapRight);
	PlayerInputComponent->BindAction(TEXT("SnapLeft"), EInputEvent::IE_Pressed, this, &AVRCharacter::OnSnapLeft);
}

