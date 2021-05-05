// Copyright Epic Games, Inc. All Rights Reserved.

#include "KartsGameMode.h"
#include "KartsPawn.h"
#include "KartsHud.h"

AKartsGameMode::AKartsGameMode()
{
	DefaultPawnClass = AKartsPawn::StaticClass();
	HUDClass = AKartsHud::StaticClass();
}
