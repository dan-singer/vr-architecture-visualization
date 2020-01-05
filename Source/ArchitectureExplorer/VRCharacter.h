// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HandController.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class AHandController* LeftController;

	UPROPERTY(VisibleAnywhere)
	class AHandController* RightController;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> HandController;

	UPROPERTY()
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* TeleportArcMesh;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* TeleportArcMaterial;

	UPROPERTY(EditDefaultsOnly)
	float MaxTeleportDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly)
	float TeleportDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly)
	float FadeDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float SnapTurnDegrees = 45.0f;

	float CurrentSnapTurnDegrees = SnapTurnDegrees;

	UPROPERTY()
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	class UMaterialInstanceDynamic* BlinkerMaterialDynamic;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsSpeed;

	FTimerHandle TimerHandle;

	bool HasDestination = false;

	void OnHorizontal(float value);
	void OnVertical(float value);
	void OnTeleport();
	void OnSnapRight();
	void OnSnapLeft();
	void SnapTurn();
	void SnapTurnEnd();
	void FadeOutAndTeleport();
	void UpdateBlinkers();
	FVector2D GetBlinkerCenter();

	void GripLeft() { LeftController->Grip(); }
	void ReleaseLeft()  { LeftController->Release(); }

	void GripRight() { RightController->Grip(); }
	void ReleaseRight() { RightController->Release(); }

	bool FindTeleportLocation(TArray<FVector>& outPath, FVector& outLocation);
	void UpdateSpline(const TArray<FVector>& worldPoints);
	void UpdateDestinationMarker();

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
