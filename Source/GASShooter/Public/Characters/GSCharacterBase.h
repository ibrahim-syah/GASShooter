// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Characters/Abilities/GSDamageable.h"
#include "GameplayTagContainer.h"
#include "GASShooter/GASShooter.h"
#include "TimerManager.h"
#include "GSCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterBaseHitReactDelegate, EGSHitReactDirection, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AGSCharacterBase*, Character);

USTRUCT(BlueprintType)
struct GASSHOOTER_API FGSDamageNumber
{
	GENERATED_USTRUCT_BODY()

	float DamageAmount;
	FVector HitLocation;

	FGameplayTagContainer Tags;

	FGSDamageNumber() {}

	FGSDamageNumber(float InDamageAmount, FGameplayTagContainer InTags, FVector InHitLocation) : DamageAmount(InDamageAmount), HitLocation(InHitLocation)
	{
		// Copy tag container
		Tags.AppendTags(InTags);
	}
};

USTRUCT(BlueprintType)
struct GASSHOOTER_API FGSKillMarker
{
	GENERATED_USTRUCT_BODY()

	FVector KilledLocation;

	FGameplayTagContainer Tags;

	FGSKillMarker() {}

	FGSKillMarker(FGameplayTagContainer InTags, FVector InKillLocation) : KilledLocation(InKillLocation)
	{
		// Copy tag container
		Tags.AppendTags(InTags);
	}
};

USTRUCT(BlueprintType)
struct GASSHOOTER_API FGSDamageIndicator
{
	GENERATED_USTRUCT_BODY()

	FVector SourceLocation;

	FGSDamageIndicator() {}

	FGSDamageIndicator(FVector InSourceLocation) : SourceLocation(InSourceLocation)
	{
		
	}
};

/**
* The base Character class for the game. Everything with an AbilitySystemComponent in this game will inherit from this class.
* This class should not be instantiated and instead subclassed.
*/
UCLASS()
class GASSHOOTER_API AGSCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGSDamageable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGSCharacterBase(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "GASShooter|GSCharacter")
	FCharacterDiedDelegate OnCharacterDied;

	// Implement IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter")
	virtual bool IsAlive() const;

	// Switch on AbilityID to return individual ability levels.
	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter")
	virtual int32 GetAbilityLevel(EGSAbilityInputID AbilityID) const;

	// Removes all CharacterAbilities. Can only be called by the Server. Removing on the Server will remove from Client too.
	virtual void RemoveCharacterAbilities();

	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter")
	virtual void FinishDying();

	virtual void AddDamageNumber(float Damage, FGameplayTagContainer DamageNumberTags, FVector HitLocation);
	virtual void AddKillMarker(FGameplayTagContainer KillMarkerTags, FVector KillLocation);
	virtual void AddDamageIndicator(FVector SourceLocation);

	// I moved perspective up the base because even a minion pawn or tank pawn etc might have a perspective before and as you possess them.
	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSHeroCharacter")
	virtual bool IsInFirstPersonPerspective() const;

	/**
	* Getters for attributes from GSAttributeSetBase
	**/

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	int32 GetCharacterLevel() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMana() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMaxMana() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetStamina() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetShield() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMaxShield() const;

	// Gets the Current value of MoveSpeed
	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMoveSpeed() const;

	// Gets the Base value of MoveSpeed
	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
	float GetMoveSpeedBaseValue() const;

	//////////////////////// Hit React
	// Set the Hit React direction
	UPROPERTY(BlueprintAssignable, Category = "GASShooter|GSCharacter")
	FCharacterBaseHitReactDelegate ShowHitReact;

	UFUNCTION(BlueprintCallable)
	EGSHitReactDirection GetHitReactDirection(const FVector& ImpactPoint);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	virtual void PlayHitReact(FGameplayTag HitDirection, AActor* DamageCauser);
	virtual void PlayHitReact_Implementation(FGameplayTag HitDirection, AActor* DamageCauser);
	virtual bool PlayHitReact_Validate(FGameplayTag HitDirection, AActor* DamageCauser);

	/**
	* Damageable interface
	*/

	/**
	* This Character can be taken down by other heroes when:
	* HP is less than 25% - to knock them down
	*/
	virtual bool IsAvailableForTakedown_Implementation(UPrimitiveComponent* TakedownComponent) const override;

	/**
	* How long to takedown with this player:
	* should be instant
	*/
	virtual float GetTakedownDuration_Implementation(UPrimitiveComponent* TakedownComponent) const override;

	/**
	* takedown:
	* activate takedown GA (plays animation)
	*/
	virtual void PreTakedown_Implementation(AActor* Takedowner, UPrimitiveComponent* TakedownComponent) override;

	/**
	* takedown:
	* apply takedown GE
	*/
	virtual void PostTakedown_Implementation(AActor* Takedowner, UPrimitiveComponent* TakedownComponent) override;

	virtual void GetPreTakedownSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const override;

	/**
	* Cancel takedown:
	* takedown - cancel takedown ability
	*/
	virtual void CancelTakedown_Implementation(UPrimitiveComponent* TakedownComponent) override;

	/**
	* Get the delegate for this Actor canceling takedown:
	* cancel being taken down if killed
	*/
	FSimpleMulticastDelegate* GetTargetCancelTakedownDelegate(UPrimitiveComponent* TakedownComponent) override;
	FSimpleMulticastDelegate TakedownCanceledDelegate;

	virtual bool IsStatusBarAvailable_Implementation() const override;
	virtual void FadeInStatusBar_Implementation() const override;
	virtual void FadeOutStatusBar_Implementation() const override;


protected:
	FGameplayTag HitDirectionFrontTag;
	FGameplayTag HitDirectionBackTag;
	FGameplayTag HitDirectionRightTag;
	FGameplayTag HitDirectionLeftTag;
	FGameplayTag DeadTag;
	FGameplayTag EffectRemoveOnDeathTag;
	FGameplayTag BeingTakendownTag;
	FGameplayTag KnockedDownTag;

	TArray<FGSDamageNumber> DamageNumberQueue;
	FTimerHandle DamageNumberTimer;

	TArray<FGSKillMarker> KillMarkerQueue;
	FTimerHandle KillMarkerTimer;

	TArray<FGSDamageIndicator> DamageIndicatorQueue;
	FTimerHandle DamageIndicatorTimer;
	
	// Reference to the ASC. It will live on the PlayerState or here if the character doesn't have a PlayerState.
	UPROPERTY()
	class UGSAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly)
	class UMotionWarpingComponent* MotionWarpingComponent;

	// Reference to the AttributeSetBase. It will live on the PlayerState or here if the character doesn't have a PlayerState.
	UPROPERTY()
	class UGSAttributeSetBase* AttributeSetBase;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|GSCharacter")
	FText CharacterName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Animation")
	UAnimMontage* ReviveMontage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Animation")
	UAnimMontage* DeathMontage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Animation")
	UAnimMontage* StunMontage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Takedown")
	bool bCanEverBeTakenDown;

	//UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Audio")
	//class USoundBase* DeathSound;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	bool bIsFirstPersonPerspective;

	// Default abilities for this Character. These will be removed on Character death and regiven if Character respawns.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Abilities")
	TArray<TSubclassOf<class UGSGameplayAbility>> CharacterAbilities;

	// Default attributes for a character for initializing on spawn/respawn.
	// This is an instant GE that overrides the values for attributes that get reset on spawn/respawn.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Abilities")
	TSubclassOf<class UGameplayEffect> DefaultAttributes;

	// These effects are only applied one time on startup
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|Abilities")
	TSubclassOf<UGameplayEffect> TakendownEffect;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSFloatingStatusBarWidget> UIFloatingStatusBarClass;

	UPROPERTY()
	class UGSFloatingStatusBarWidget* UIFloatingStatusBar;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|UI")
	class UWidgetComponent* UIFloatingStatusBarComponent;

	UPROPERTY(EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSDamageTextWidgetComponent> DamageNumberClass;

	UPROPERTY(EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSDamageMarkerWidgetComponent> DamageMarkerClass;

	UPROPERTY(EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSKillMarkerWidgetComponent> KillMarkerClass;

	UPROPERTY(EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSHUDDamageIndicator> DamageIndicatorClass;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Grant abilities on the Server. The Ability Specs will be replicated to the owning client.
	virtual void AddCharacterAbilities();

	// Initialize the Character's attributes. Must run on Server but we run it on Client too
	// so that we don't have to wait. The Server's replication to the Client won't matter since
	// the values should be the same.
	virtual void InitializeAttributes();

	virtual void AddStartupEffects();

	virtual void ShowDamageNumber();

	virtual void ShowKillMarker();

	virtual void ShowDamageIndicator();


	/**
	* Setters for Attributes. Only use these in special cases like Respawning, otherwise use a GE to change Attributes.
	* These change the Attribute's Base Value.
	*/

	virtual void SetHealth(float Health);
	virtual void SetMana(float Mana);
	virtual void SetStamina(float Stamina);
	virtual void SetShield(float Shield);
};
