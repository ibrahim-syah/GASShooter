// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/CollisionProfile.h"
#include "GSAT_WaitDamageableTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitDamageableTargetDelegate, const FGameplayAbilityTargetDataHandle&, Data);

/**
 * Performs a line trace on a timer, looking for a Pawn that implements IGSDamageable that is available for takedown.
 * The StartLocations are hardcoded for GASShooter since we can be in first and third person so we have to check every time
 * we trace. If you only have one start location, you should make it more generic with a parameter on your AbilityTask node.
 */
UCLASS()
class GASSHOOTER_API UGSAT_WaitDamageableTarget : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitDamageableTargetDelegate FoundNewDamageableTarget;

	UPROPERTY(BlueprintAssignable)
	FWaitDamageableTargetDelegate LostDamageableTarget;

	/**
	* Traces a line on a timer looking for DamageableTargets.
	* @param MaxRange How far to trace.
	* @param TimerPeriod Period of trace timer.
	* @param bShowDebug Draws debug lines for traces.
	*/
	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", HideSpawnParms = "Instigator"), Category = "Ability|Tasks")
	static UGSAT_WaitDamageableTarget* WaitForDamageableTarget(UGameplayAbility* OwningAbility, FName TaskInstanceName, FCollisionProfileName TraceProfile, float MaxRange = 200.0f, float TimerPeriod = 0.1f, bool bShowDebug = true);

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
	void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params, bool bLookForDamageableActor) const;

	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition) const;

	UFUNCTION()
	void PerformTrace();

	FGameplayAbilityTargetDataHandle MakeTargetData(const FHitResult& HitResult) const;
	
};
