// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GSDamageMarkerWidgetComponent.generated.h"

/**
 * For the hit marker/damage marker in the crosshair when damaging something
 */
UCLASS()
class GASSHOOTER_API UGSDamageMarkerWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDamageMarker(const FGameplayTagContainer& Tags);
};
