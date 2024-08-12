// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "GSDamageable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGSDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for Pawns that can be takendown through the GameplayAbilitySystem. Very similar to GSInteractable. 
 * This is useful for tracing actor and checking if we can perform a takedown ability, or display their status bar.
 * Could I use GSInteractable instead of creating a new interface? maybe, idk man.
 */
class GASSHOOTER_API IGSDamageable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/**
	* Can this pawn be taken down? this would return true if it bCanEverBeTakenDown is true and health is <= 25%
	*
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate Takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	bool IsAvailableForTakedown(UPrimitiveComponent* TakedownComponent) const;
	virtual bool IsAvailableForTakedown_Implementation(UPrimitiveComponent* TakedownComponent) const;

	/**
	* How long does the player need to hold down the takedown button to takedown this pawn?
	*
	* @param TakedownComponent UPrimitiveComponent in case a Pawn has many separate Takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	float GetTakedownDuration(UPrimitiveComponent* TakedownComponent) const;
	virtual float GetTakedownDuration_Implementation(UPrimitiveComponent* TakedownComponent) const;

	/**
	* Should we sync and who should sync before calling PreTakedown()? Defaults to false and OnlyServerWait.
	* OnlyServerWait - client predictively calls PreTakedown().
	* OnlyClientWait - client waits for server to call PreTakedown(). This is useful if we are activating an ability
	* on another ASC (player) and want to sync actions or animations with our Takedown Duration timer.
	* BothWait - client and server wait for each other before calling PreTakedown().
	*
	* Player revive uses OnlyClientWait so that the player reviving is in sync with the server since we can't locally
	* predict an ability run on another player. The downed player's reviving animation will be in sync with the local
	* player's Takedown Duration Timer.
	*
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate Takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	void GetPreTakedownSyncType(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const;
	virtual void GetPreTakedownSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const;

	/**
	* Should we sync and who should sync before calling PostTakedown()? Defaults to false and OnlyServerWait.
	* OnlyServerWait - client predictively calls PostTakedown().
	* OnlyClientWait - client waits for server to call PostTakedown().
	* BothWait - client and server wait for each other before calling PostTakedown().
	*
	* Player revive uses OnlyServerWait so that the client isn't stuck waiting for the server after the Takedown Duration
	* ends. Revive's PostTakedown() will only run code on the server so it's fine for the client to be "finished" ahead of
	* the server.
	*
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate Takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	void GetPostTakedownSyncType(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const;
	void GetPostTakedownSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const;

	/**
	* Takedown with this Actor. This will call before starting the Takedown Duration timer. This might do things, apply
	* (predictively or not) GameplayEffects, trigger (predictively or not) GameplayAbilities, etc.
	*
	* You can use this function to grant abilities that will be predictively activated on PostTakedown() to hide the
	* AbilitySpec replication time.
	*
	* If you want to do something predictively, you can get the ASC from the TakedowningActor and use its
	* ScopedPredictionKey.
	*
	* Player revives use PreTakedown() to trigger a ability that plays an animation that lasts the same duration as
	* the Takedown Duration. If this ability finishes, it will revive the player in PostTakedown().
	*
	* @param TakedowningActor The Actor performing takedown against this Actor. It will be the AvatarActor from a GameplayAbility.
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	void PreTakedown(AActor* TakedowningActor, UPrimitiveComponent* TakedownComponent);
	virtual void PreTakedown_Implementation(AActor* TakedowningActor, UPrimitiveComponent* TakedownComponent) {};

	/**
	* Takedown with this Actor. This will call after the Takedown Duration timer completes. This might do things, apply
	* (predictively or not) GameplayEffects, trigger (predictively or not) GameplayAbilities, etc.
	*
	* If you want to do something predictively, you can get the ASC from the TakedowningActor and use its
	* ScopedPredictionKey.
	*
	* If you need to trigger a GameplayAbility predictively, the player's ASC needs to have been granted the ability
	* ahead of time. If you don't want to grant every possible predictive ability at game start, you can hide the time
	* needed to replicate the AbilitySpec inside the time needed to Takedown by granted it in PreTakedown().
	*
	* @param TakedowningActor The Actor performing takedown against this Actor. It will be the AvatarActor from a GameplayAbility.
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	void PostTakedown(AActor* TakedowningActor, UPrimitiveComponent* TakedownComponent);
	virtual void PostTakedown_Implementation(AActor* TakedowningActor, UPrimitiveComponent* TakedownComponent) {};

	/**
	* Cancel an ongoing takedown, typically anything happening in PreTakedown() while waiting on the Takedown Duration
	* Timer.
	*
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate takedownable areas.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable")
	void CancelTakedown(UPrimitiveComponent* TakedownComponent);
	virtual void CancelTakedown_Implementation(UPrimitiveComponent* TakedownComponent) {};

	/**
	* Returns a delegate for GA_Takedown to bind to that fires when this Pawn is canceling the takedown (e.g. died).
	*
	* @param TakedownComponent UPrimitiveComponent in case an Actor has many separate takedownable areas.
	*/
	virtual FSimpleMulticastDelegate* GetTargetCancelTakedownDelegate(UPrimitiveComponent* TakedownComponent);

	/**
	* Registers an Actor performing takedown with this Takedownable. Used to send a GameplayEvent to them when this Takedownable
	* wishes to cancel takedown prematurely (e.g. a player performing the takedown dies mid-takedown or vice-versa). Not meant to be overriden.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable|Do Not Override")
	void RegisterTakedowner(UPrimitiveComponent* TakedownComponent, AActor* TakedowningActor);
	void RegisterTakedowner_Implementation(UPrimitiveComponent* TakedownComponent, AActor* TakedowningActor);

	/**
	* Unregisters an Actor performing takedown with this Takedownable. Not meant to be overriden.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable|Do Not Override")
	void UnregisterTakedowner(UPrimitiveComponent* TakedownComponent, AActor* TakedowningActor);
	void UnregisterTakedowner_Implementation(UPrimitiveComponent* TakedownComponent, AActor* TakedowningActor);

	/**
	* Takedownable (or an external Actor, not the actor performing the takedown) wants to cancel the takedown (e.g. player performing the takedown
	* dies mid-takedown or vice-versa). Not meant to be overriden.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Takedownable|Do Not Override")
	void TakedownableCancelTakedown(UPrimitiveComponent* TakedownComponent);
	void TakedownableCancelTakedown_Implementation(UPrimitiveComponent* TakedownComponent);




	/**
	* Does this pawn have a status bar?
	*
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "StatusBar")
	bool IsStatusBarAvailable() const;
	virtual bool IsStatusBarAvailable_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "StatusBar")
	void FadeInStatusBar() const;
	virtual void FadeInStatusBar_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "StatusBar")
	void FadeOutStatusBar() const;
	virtual void FadeOutStatusBar_Implementation() const;

protected:
	TMap<UPrimitiveComponent*, TArray<AActor*>> Takedowners;
};
