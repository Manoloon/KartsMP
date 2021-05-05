// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "KartsHud.generated.h"


UCLASS(config = Game)
class AKartsHud : public AHUD
{
	GENERATED_BODY()

public:
	AKartsHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
