// Fill out your copyright notice in the Description page of Project Settings.


#include "KartMoveComponent.h"
#include "GameFrameWork/GameState.h"

// Sets default values for this component's properties
UKartMoveComponent::UKartMoveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UKartMoveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UKartMoveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FVector UKartMoveComponent::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UKartMoveComponent::GetRollingResistance()
{
	float Gravity = GetWorld()->GetDefaultGravityZ() / 100;
	float NormalForce = Mass / Gravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void UKartMoveComponent::CalculateTranslation(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;
	FHitResult BlockResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &BlockResult);
	if(BlockResult.bBlockingHit)
	{
		Velocity = FVector::ZeroVector;
	}
}

void UKartMoveComponent::CalculateRotation(float DeltaTime, float newSteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	// TODO : Cambiar ese 25 por la mitad de la velocidad promedio.
	if(Velocity.Size() > 25)
	{
		//TODO : Cambiar esos numeros por datos sobre el radio de giro.
		MinTurningRadius = FMath::FInterpTo(10, 100, DeltaTime, 2.0f);
	}
	float RotationAngle = DeltaLocation / MinTurningRadius * newSteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	GetOwner()->AddActorWorldRotation(RotationDelta);
}

FKartMovement UKartMoveComponent::CreateMoveAction(float DeltaTime)
{
	FKartMovement KartMove;
	KartMove.DeltaTime = DeltaTime;
	KartMove.Throttle = Throttle;
	KartMove.SteeringThrow = SteeringThrow;
	KartMove.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	return KartMove;
}

void UKartMoveComponent::ClearMoveAction(FKartMovement LastMove)
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

void UKartMoveComponent::SimulateMove(const FKartMovement& newMove)
{
	FVector Force = (GetOwner()->GetActorForwardVector() * MaxDrivingForce * newMove.Throttle);
	Force += GetAirResistance();
	Force += GetRollingResistance();
	// Accelaration = DeltaVelocity / DeltaTime
	FVector Accelaration = Force / Mass;
	// Velocity = DeltaLocation / DeltaTime
	Velocity = Velocity + Accelaration * newMove.DeltaTime;
	CalculateRotation(newMove.DeltaTime, newMove.SteeringThrow);
	CalculateTranslation(newMove.DeltaTime);
}

void UKartMoveComponent::AddToUnacknowledgeMoves(FKartMovement newMove)
{
	UnacknowledgeMoves.Add(newMove);
	UE_LOG(LogTemp, Warning, TEXT("Move queue %d"), UnacknowledgeMoves.Num());
}

