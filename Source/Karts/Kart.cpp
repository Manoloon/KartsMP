// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"
#include "GameFrameWork/GameState.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AKart::AKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetBoxExtent(FVector(220.0f,90.0f,40.0f));
	RootComponent = BoxCollision;
	KartMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("KartMesh"));
	KartMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	KartMesh->SetupAttachment(BoxCollision);
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(BoxCollision);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	
	bReplicates = true;
}

void AKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKart, Velocity);
	DOREPLIFETIME(AKart, ServerState);
}

// Called to bind functionality to input
void AKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AKart::MoveRight);
	// 	PlayerInputComponent->BindAxis("LookUp");
	// 	PlayerInputComponent->BindAxis("LookRight");
}

void AKart::MoveForward(float Val)
{
	Throttle = Val;
}

void AKart::MoveRight(float Val)
{
	SteeringThrow = Val;
}

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();	
	NetUpdateFrequency = 3;
}

FString GetRoleName(ENetRole newRole)
{
	switch (newRole)
	{
	case ROLE_None:
		return ("NONE");
	case ROLE_SimulatedProxy:
		return("SimulatedProxy");
	case ROLE_AutonomousProxy:
		return("AutonomousProxy");
	case ROLE_Authority:
		return("Authority");
	default:
		return("ERROR");
	}
}

// Called every frame
void AKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		FKartMove newMove = CreateMoveAction(DeltaTime);
		UnacknowledgeMoves.Add(newMove);
		UE_LOG(LogTemp, Warning, TEXT("Move queue %d"), UnacknowledgeMoves.Num());
		SimulateMove(newMove);
		Server_SendKartMove(newMove);
	}
	// WE ARE THE SERVER AND IN CONTROL OF THE PAWN
	if(IsLocallyControlled())
	{
		FKartMove newMove = CreateMoveAction(DeltaTime);
		Server_SendKartMove(newMove);
	}
	if(GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}
	DrawDebugString(GetWorld(), FVector(0.0, 0.0, 100.0f), GetRoleName(GetLocalRole()),this, FColor::Green, DeltaTime, false);
}


void AKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
	ClearMoveAction(ServerState.LastMove);
	for(const FKartMove& move : UnacknowledgeMoves)
	{
		SimulateMove(move);
	}
}

void AKart::CalculateTranslation(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;
	
	FHitResult BlockResult;
	AddActorWorldOffset(Translation, true,&BlockResult);

	if(BlockResult.bBlockingHit)
	{
		Velocity = FVector(0.0);
	}
}

void AKart::CalculateRotation(float DeltaTime,float newSteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
 	if(Velocity.Size() > 25)
 	{
 		MinTurningRadius = FMath::FInterpTo(10, 100, DeltaTime, 1.0f);
 	}
	
	float RotationAngle = DeltaLocation / MinTurningRadius * newSteeringThrow;
	FQuat RotationDelta(GetActorUpVector(),RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
}

FKartMove AKart::CreateMoveAction(float DeltaTime)
{
	FKartMove KartMove;
	KartMove.DeltaTime = DeltaTime;
	KartMove.Throttle = Throttle;
	KartMove.SteeringThrow = SteeringThrow;
	KartMove.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();	
	return KartMove;
}

void AKart::ClearMoveAction(FKartMove LastMove)
{
	TArray<FKartMove> TempMoves;
	for(const FKartMove& move : UnacknowledgeMoves)
	{
		if(move.Time > LastMove.Time)
		{
			TempMoves.Add(move);
		}
	}
	UnacknowledgeMoves = TempMoves;
	//UE_LOG(LogTemp, Warning, TEXT("Move queue %d"), TempMoves.Num());
}

void AKart::SimulateMove(const FKartMove& newMove)
{
	FVector Force = (GetActorForwardVector() * MaxDrivingForce * newMove.Throttle);
	Force += GetAirResistance();
	Force += GetRollingResistance();
	// Accelaration = DeltaVelocity / DeltaTime
	FVector Accelaration = Force / Mass;
	// Velocity = DeltaLocation / DeltaTime
	Velocity = Velocity + Accelaration * newMove.DeltaTime;
	CalculateRotation(newMove.DeltaTime,newMove.SteeringThrow);
	CalculateTranslation(newMove.DeltaTime);
}

FVector AKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AKart::GetRollingResistance()
{
	float Gravity = GetWorld()->GetDefaultGravityZ() / 100;
	float NormalForce = Mass / Gravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

// RPC

void AKart::Server_SendKartMove_Implementation(FKartMove newMove)
{
	SimulateMove(newMove);
	ServerState.LastMove = newMove;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

bool AKart::Server_SendKartMove_Validate(FKartMove newMove)
{
	return true;
}