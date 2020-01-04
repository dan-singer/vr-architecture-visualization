// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
	class UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* RightController;

	UPROPERTY(EditDefaultsOnly)
	float MaxTeleportDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly)
	float FadeDuration = 1.0f;

	UPROPERTY()
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	class UMaterialInstanceDynamic* BlinkerMaterialDynamic;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsSpeed;

	FTimerHandle TimerHandle;


	void OnHorizontal(float value);
	void OnVertical(float value);
	void OnTeleport();
	void FadeOutAndTeleport();
	void UpdateBlinkers();
	FVector2D GetBlinkerCenter();


	bool FindTeleportLocation(FVector& outLocation);
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
