// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSHUDDamageIndicator.generated.h"

/**
 * 
 */
UCLASS()
class GASSHOOTER_API UGSHUDDamageIndicator : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	FVector HitLocation;

	UFUNCTION(BlueprintCallable)
	float UpdateAngle(FVector ForwardVector, FVector ActorLocationVector);
};
