// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/CollisionProfile.h"
#include "GSAT_WaitStatusBarTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitStatusBarTargetDelegate, const FGameplayAbilityTargetDataHandle&, Data);
/**
 * 
 */
UCLASS()
class GASSHOOTER_API UGSAT_WaitStatusBarTarget : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitStatusBarTargetDelegate FoundNewStatusBarTarget;

	UPROPERTY(BlueprintAssignable)
	FWaitStatusBarTargetDelegate LostStatusBarTarget;

	/**
	* Traces a line on a timer looking for StatusBarTargets.
	* @param MaxRange How far to trace.
	* @param TimerPeriod Period of trace timer.
	* @param bShowDebug Draws debug lines for traces.
	*/
	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", HideSpawnParms = "Instigator"), Category = "Ability|Tasks")
	static UGSAT_WaitStatusBarTarget* WaitForStatusBarTarget(UGameplayAbility* OwningAbility, FName TaskInstanceName, FCollisionProfileName TraceProfile, float MaxRange = 200.0f, float TimerPeriod = 0.1f, bool bShowDebug = true);

	virtual void Activate() override;

protected:
	FGameplayAbilityTargetingLocationInfo StartLocation;
	FGameplayAbilityTargetingLocationInfo StartLocation1P;
	FGameplayAbilityTargetingLocationInfo StartLocation3P;

	float MaxRange;

	float TimerPeriod;

	bool bShowDebug;

	bool bTraceAffectsAimPitch;

	FCollisionProfileName TraceProfile;

	FGameplayAbilityTargetDataHandle TargetData;

	FTimerHandle TraceTimerHandle;

	virtual void OnDestroy(bool AbilityEnded) override;

	/** Traces as normal, but will manually filter all hit actors */
	void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params, bool bLookForStatusBarActor) const;

	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition) const;

	UFUNCTION()
	void PerformTrace();

	FGameplayAbilityTargetDataHandle MakeTargetData(const FHitResult& HitResult) const;
	
};
