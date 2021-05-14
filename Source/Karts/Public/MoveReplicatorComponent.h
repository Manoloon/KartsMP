// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KartMoveComponent.h"
#include "MoveReplicatorComponent.generated.h"

USTRUCT()
struct FServerState
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY()
		FVector Velocity;
	UPROPERTY()
		FTransform Transform;
	UPROPERTY()
		FKartMovement LastMove;
};

struct FHermitSpline
{
	FVector ClientStartLocation, StartDerivative, ClientTargetLocation, TargetDerivative;
	FQuat ClientStartRotation, ClientTargetRotation;
	FVector InterpolateDerivative(float LerpRatio) const 
	{
		return FMath::CubicInterpDerivative(ClientStartLocation, StartDerivative, ClientTargetLocation, TargetDerivative, LerpRatio);
	}
	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(ClientStartLocation, StartDerivative, ClientTargetLocation, TargetDerivative, LerpRatio);
	}
	FQuat InterpolateRotation(float LerpRatio) const 
	{
		return FQuat::Slerp(ClientStartRotation, ClientTargetRotation, LerpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KARTS_API UMoveReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMoveReplicatorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetMeshOffsetComp(USceneComponent* newComp) { KartMeshOffsetRootComp = newComp; };
private:
	UPROPERTY()
		UKartMoveComponent* KartMoveComponent = nullptr;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdate;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;
	FVector ClientNextLocation;
	FQuat ClientNextRotation;
	float ClientSimulatedTime;
	class USceneComponent* KartMeshOffsetRootComp;
	FHermitSpline Spline;
	float GetVelocityToDerivative() { return ClientTimeBetweenLastUpdate * 100; };
	TArray<FKartMovement> UnacknowledgeMoves;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
		FServerState ServerState;

	UFUNCTION()
		void OnRep_ServerState();

	UFUNCTION()
		void AutonomousProxy_OnRep_ServerState();
	UFUNCTION()
		void SimulatedProxy_OnRep_ServerState();

	void ClearUnacknowledgeMoves(FKartMovement LastMove);

	void UpdateServerState(const FKartMovement& newLastMove);
	void ClientTick(float DeltaTime);


	// network RPC
	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendKartMove(FKartMovement newMove);
};
