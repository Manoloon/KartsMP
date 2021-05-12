// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Public/KartMoveComponent.h"
#include "Kart.generated.h"

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
	UPROPERTY(EditAnywhere)
		UKartMoveComponent* KartMoveComp;

private:

	// network stuff	

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
		FServerState ServerState;

	UFUNCTION()
		void OnRep_ServerState();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float Val);
	void MoveRight(float Val);

	// network RPC
	UFUNCTION(Server, Reliable, WithValidation)
		void Server_SendKartMove(FKartMovement newMove);
};
