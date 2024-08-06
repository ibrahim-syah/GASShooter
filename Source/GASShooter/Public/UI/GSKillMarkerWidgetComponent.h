// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GSKillMarkerWidgetComponent.generated.h"

/**
 * 
 */
UCLASS()
class GASSHOOTER_API UGSKillMarkerWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetKillMarker(const FGameplayTagContainer& Tags);
};
