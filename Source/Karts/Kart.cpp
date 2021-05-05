// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Camera/CameraComponent.h"

// Sets default values
AKart::AKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetBoxExtent(FVector(220.0f,90.0f,40.0f));
	RootComponent = BoxCollision;
	KarMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("KartMesh"));
	//KarMesh->SetLocation(FVector(0.0f,0.0f,-40.0f));
	KarMesh->SetupAttachment(BoxCollision);
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(BoxCollision);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	

}

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();	
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
	Velocity = Velocity + (Accelaration * DeltaTime);

	CalculateRotation(DeltaTime);
	CalculateTranslation(DeltaTime);
}

void AKart::CalculateTranslation(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime;
	
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
	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
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
	Throttle = 100 * Val;
}

void AKart::MoveRight(float Val)
{
	SteeringThrow = Val;
}

