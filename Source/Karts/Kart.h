// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Kart.generated.h"

USTRUCT()
struct FKartMove
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY()
		float Throttle;
	UPROPERTY()
		float SteeringThrow;
	UPROPERTY()
		float DeltaTime;
	UPROPERTY()
		float Time;
};
USTRUCT()
struct FKartState
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY()
		FVector Velocity;
	UPROPERTY()
		FTransform Transform;
	UPROPERTY()
		FKartMove LastMove;
};

UCLASS()
class KARTS_API AKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// box collision como base.
	UPROPERTY(EditAnywhere)
		class UBoxComponent* BoxCollision;
	UPROPERTY(EditAnywhere)
		class USpringArmComponent* SpringArm;
	UPROPERTY(EditAnywhere)
		class UCameraComponent* Camera;
	UPROPERTY(EditAnywhere)
		class USkeletalMeshComponent* KartMesh;
	// Minimum turning radius -- 10 metros.
	UPROPERTY(EditAnywhere)
		float MinTurningRadius = 10.0f;

private:

	uint32 Mass = 1000;
	UPROPERTY(Replicated)
		FVector Velocity;
	UPROPERTY(Replicated)
		float Throttle;
	// 10000 / 1000 = 10 metros por segundo.
	uint32 MaxDrivingForce = 10000; 
	UPROPERTY(Replicated)
		float SteeringThrow;
	//DragCoefficient is indeed unitless! 
	// 0.3f - 0.5f is generally good to use. 
	//0.3f - Tanks, 0.5f - Automobiles
	float DragCoefficient = 0.5f;
	float RollingResistanceCoefficient = 0.015f;
	//The DragArea that is (cm^2). Higher means more drag
	float DragArea = 20000.0f;

	void CalculateTranslation(float DeltaTime);
	void CalculateRotation(float DeltaTime);
	FVector GetAirResistance();
	FVector GetRollingResistance();

	// network stuff
	UPROPERTY(ReplicatedUsing = OnRep_PawnTransform)
		FTransform ReplicatePawnTransform;
	

	UFUNCTION()
		void OnRep_PawnTransform();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float Val);
	void MoveRight(float Val);

	// network
	UFUNCTION(Server,Reliable,WithValidation)
	void Server_MoveForward(float Val);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Val);
};
