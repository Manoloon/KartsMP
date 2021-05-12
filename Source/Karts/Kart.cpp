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
	KartMoveComp = CreateDefaultSubobject<UKartMoveComponent>(TEXT("KartMoveComp"));
	
	bReplicates = true;
}

void AKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AKart, Velocity);
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
	KartMoveComp->SetThrottle(Val);
}

void AKart::MoveRight(float Val)
{
	KartMoveComp->SetSteeringThrow(Val);
}

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();	

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
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
	if (!KartMoveComp) { return; }
	if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		FKartMovement newMove = KartMoveComp->CreateMoveAction(DeltaTime);
		KartMoveComp->SimulateMove(newMove);
		KartMoveComp->AddToUnacknowledgeMoves(newMove);	
		
		Server_SendKartMove(newMove);
	}
	// WE ARE THE SERVER AND IN CONTROL OF THE PAWN
	if(IsLocallyControlled())
	{
		FKartMovement newMove = KartMoveComp->CreateMoveAction(DeltaTime);
		Server_SendKartMove(newMove);
	}
	if(GetLocalRole() == ROLE_SimulatedProxy)
	{
		KartMoveComp->SimulateMove(ServerState.LastMove);
	}
	DrawDebugString(GetWorld(), FVector(0.0, 0.0, 100.0f), GetRoleName(GetLocalRole()),this, FColor::Green, DeltaTime, false);
}


void AKart::OnRep_ServerState()
{
	if (KartMoveComp)
	{
		SetActorTransform(ServerState.Transform);
		KartMoveComp->GetVelocity() = ServerState.Velocity;
		KartMoveComp->ClearMoveAction(ServerState.LastMove);
		for(const FKartMovement& move : KartMoveComp->GetUnacknowledgeMoves())
		{
			KartMoveComp->SimulateMove(move);
		}
	}

}

// RPC

void AKart::Server_SendKartMove_Implementation(FKartMovement newMove)
{
	if (!KartMoveComp) { return; }
	KartMoveComp->SimulateMove(newMove);
	ServerState.LastMove = newMove;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = KartMoveComp->GetVelocity();
}

bool AKart::Server_SendKartMove_Validate(FKartMovement newMove)
{
	return true;
}