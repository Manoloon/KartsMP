// Fill out your copyright notice in the Description page of Project Settings.

#include "MoveReplicatorComponent.h"
#include "KartMoveComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UMoveReplicatorComponent::UMoveReplicatorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UMoveReplicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	KartMoveComponent = GetOwner()->FindComponentByClass<UKartMoveComponent>();	
}


// Called every frame
void UMoveReplicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (KartMoveComponent == nullptr) { return; }

	FKartMovement newLastMove = KartMoveComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgeMoves.Add(newLastMove);
		Server_SendKartMove(newLastMove);
	}
	// WE ARE THE SERVER AND IN CONTROL OF THE PAWN
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(newLastMove);
	}
	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		KartMoveComponent->SimulateMove(ServerState.LastMove);
	}
}

// RPC

void UMoveReplicatorComponent::Server_SendKartMove_Implementation(FKartMovement newMove)
{
	if (KartMoveComponent == nullptr) { return; }
	KartMoveComponent->SimulateMove(newMove);
	UpdateServerState(newMove);
}

bool UMoveReplicatorComponent::Server_SendKartMove_Validate(FKartMovement newMove)
{
	return true;
}

void UMoveReplicatorComponent::OnRep_ServerState()
{
	if (KartMoveComponent == nullptr) { return; }

	GetOwner()->SetActorTransform(ServerState.Transform);
	KartMoveComponent->SetVelocity(ServerState.Velocity);

	ClearUnacknowledgeMoves(ServerState.LastMove);

	for (const FKartMovement& move : UnacknowledgeMoves)
	{
		KartMoveComponent->SimulateMove(move);
	}
}

void UMoveReplicatorComponent::ClearUnacknowledgeMoves(FKartMovement LastMove)
{
	TArray<FKartMovement> TempMoves;
	for (const FKartMovement& move : UnacknowledgeMoves)
	{
		if (move.Time > LastMove.Time)
		{
			TempMoves.Add(move);
		}
	}
	UnacknowledgeMoves = TempMoves;
}

void UMoveReplicatorComponent::UpdateServerState(const FKartMovement& newLastMove)
{
	if (KartMoveComponent)
	{
		ServerState.LastMove = newLastMove;
		ServerState.Transform = GetOwner()->GetActorTransform();
		ServerState.Velocity = KartMoveComponent->GetVelocity();
	}
}

void UMoveReplicatorComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMoveReplicatorComponent, ServerState);
}