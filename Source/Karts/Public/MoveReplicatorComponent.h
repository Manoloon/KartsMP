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

private:
	UPROPERTY()
		UKartMoveComponent* KartMoveComponent = nullptr;

	TArray<FKartMovement> UnacknowledgeMoves;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
		FServerState ServerState;

	UFUNCTION()
		void OnRep_ServerState();

	void ClearUnacknowledgeMoves(FKartMovement LastMove);

	void UpdateServerState(const FKartMovement& newLastMove);

	// network RPC
	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendKartMove(FKartMovement newMove);
};
