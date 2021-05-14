// Fill out your copyright notice in the Description page of Project Settings.

#include "MoveReplicatorComponent.h"
#include "KartMoveComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Quat.h"
#include "GameFrameWork/GameState.h"
#include "Gameframework/Actor.h"
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
		ClientTick(DeltaTime);
	}
}

void UMoveReplicatorComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;
	if (ClientTimeBetweenLastUpdate < KINDA_SMALL_NUMBER) { return; }
	
	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdate;
	
	Spline.ClientStartLocation = ClientStartTransform.GetLocation();
	Spline.ClientTargetLocation = ServerState.Transform.GetLocation();
	Spline.ClientStartRotation = ClientStartTransform.GetRotation();
	Spline.ClientTargetRotation = ServerState.Transform.GetRotation();

	Spline.StartDerivative = ClientStartVelocity * GetVelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * GetVelocityToDerivative();

	FVector ClientNextVelocity = Spline.InterpolateDerivative(LerpRatio) / GetVelocityToDerivative();
	if (KartMeshOffsetRootComp)
	{
		KartMeshOffsetRootComp->SetWorldLocation(Spline.InterpolateLocation(LerpRatio));
		KartMeshOffsetRootComp->SetWorldRotation(Spline.InterpolateRotation(LerpRatio));
	}
	if (KartMoveComponent)
	{
		KartMoveComponent->SetVelocity(ClientNextVelocity);
	}
}
// RPC

void UMoveReplicatorComponent::Server_SendKartMove_Implementation(FKartMovement newMove)
{
	if (KartMoveComponent == nullptr) { return; }
	KartMoveComponent->SimulateMove(newMove);
	ClientSimulatedTime += newMove.Time;
	UpdateServerState(newMove);
}

bool UMoveReplicatorComponent::Server_SendKartMove_Validate(FKartMovement newMove)
{
	float ProposedTime = ClientSimulatedTime + newMove.Time;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	if(!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("CHEAT : Client time way ahead of server"));
		return false;
	}
	if(!newMove.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CHEAT : Client have throttle And/Or steeringthrow wrong"));
		return false;
	}
	// previene de cheat : multiplicar el throttle o el steeringThrow. 
	return true;
}

void UMoveReplicatorComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UMoveReplicatorComponent::AutonomousProxy_OnRep_ServerState()
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

void UMoveReplicatorComponent::SimulatedProxy_OnRep_ServerState()
{
	if (KartMoveComponent == nullptr) { return; }
	
	ClientTimeBetweenLastUpdate = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;
	if(KartMeshOffsetRootComp)
	{
		ClientStartTransform = KartMeshOffsetRootComp->GetComponentTransform();
	}

	ClientStartVelocity = KartMoveComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
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
		if(KartMeshOffsetRootComp)
		{
			ServerState.Transform = KartMeshOffsetRootComp->GetComponentTransform();
		}
		
		ServerState.Velocity = KartMoveComponent->GetVelocity();
	}
}



void UMoveReplicatorComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMoveReplicatorComponent, ServerState);
}