// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"
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
	DOREPLIFETIME(AKart, ReplicatePawnTransform);
	DOREPLIFETIME(AKart, Velocity);
	DOREPLIFETIME(AKart, SteeringThrow);
	DOREPLIFETIME(AKart, Throttle);
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

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();	
	NetUpdateFrequency = 1;
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

	FVector Force =( GetActorForwardVector() * MaxDrivingForce * Throttle);
	Force += GetAirResistance();
	Force += GetRollingResistance();
	// Accelaration = DeltaVelocity / DeltaTime
	FVector Accelaration = Force / Mass;
	// Velocity = DeltaLocation / DeltaTime
	Velocity = Velocity + Accelaration * DeltaTime;
	CalculateRotation(DeltaTime);
	CalculateTranslation(DeltaTime);
	DrawDebugString(GetWorld(), FVector(0.0, 0.0, 100.0f), GetRoleName(GetLocalRole()),this, FColor::Green, DeltaTime, false);
 	if(HasAuthority())
 	{
 		ReplicatePawnTransform = GetActorTransform();
 	}
}

void AKart::OnRep_PawnTransform()
{
	SetActorTransform(ReplicatePawnTransform);
}

void AKart::CalculateTranslation(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;
	
	FHitResult BlockResult;
	AddActorWorldOffset(Translation, true,&BlockResult);
	//UE_LOG(LogTemp, Warning, TEXT("Log Velocity %f"), Translation.X);
	if(BlockResult.bBlockingHit)
	{
		Velocity = FVector(0.0);
	}
}

void AKart::CalculateRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
 	if(Velocity.Size() > 25)
 	{
 		MinTurningRadius = FMath::FInterpTo(10, 100, DeltaTime, 1.0f);
 	}
	
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(),RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
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


void AKart::MoveForward(float Val)
{
	Throttle = Val;
	Server_MoveForward(Val);
}

void AKart::MoveRight(float Val)
{
	SteeringThrow = Val;
	Server_MoveRight(Val);
}

// NETWORK

void AKart::Server_MoveForward_Implementation(float Val)
{
	Throttle = Val;
}

bool AKart::Server_MoveForward_Validate(float Val)
{
	return FMath::Abs(Val) <=1;
}



void AKart::Server_MoveRight_Implementation(float Val)
{
	SteeringThrow = Val;
}

bool AKart::Server_MoveRight_Validate(float Val)
{
	return FMath::Abs(Val) <=1;
}