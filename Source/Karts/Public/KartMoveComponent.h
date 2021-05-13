// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KartMoveComponent.generated.h"

USTRUCT()
struct FKartMovement
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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KARTS_API UKartMoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UKartMoveComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	UPROPERTY(EditAnywhere)
		uint32 MaxDrivingForce = 10000;
	//DragCoefficient is indeed unitless! 
	// 0.3f - 0.5f is generally good to use. 
	//0.3f - Tanks, 0.5f - Automobiles
	UPROPERTY(EditAnywhere)
		float DragCoefficient = 0.5f;
	UPROPERTY(EditAnywhere)
		float RollingResistanceCoefficient = 0.015f;
	//The DragArea that is (cm^2). Higher means more drag
	UPROPERTY(EditAnywhere)
		float DragArea = 20000.0f;
	// Minimum turning radius -- 10 metros.
	UPROPERTY(EditAnywhere)
		float MinTurningRadius = 10.0f;

private:

	uint32 Mass = 1000;
	//UPROPERTY(Replicated)
		FVector Velocity;
	//TArray<FKartMovement> UnacknowledgeMoves;
	float Throttle;
	float SteeringThrow;
	// 10000 / 1000 = 10 metros por segundo.

	FKartMovement LastMove;

private:
	FVector GetAirResistance();
	FVector GetRollingResistance();
	void CalculateTranslation(float DeltaTime);
	void CalculateRotation(float DeltaTime, float newSteeringThrow);

public:
	void SimulateMove(const FKartMovement& newMove);
	FKartMovement GetLastMove() { return LastMove; };
	FVector GetVelocity() const { return Velocity; };
	void SetVelocity(FVector newVelocity) { Velocity = newVelocity; };
	void SetThrottle(float Val) { Throttle = Val; };
	void SetSteeringThrow(float Val) { SteeringThrow = Val; };
	FKartMovement CreateUnacknowledgeMove(float DeltaTime);
};
