// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Characters/GSCharacterBase.h"
#include "Characters/Abilities/GSInteractable.h"
#include "GameplayEffectTypes.h"
#include "EnhancedInputComponent.h"
#include "GSHeroCharacter.generated.h"

class AGSWeapon;
class UGameplayEffect;
class UTimelineComponent;

UENUM(BlueprintType)
enum class EGSHeroWeaponState : uint8
{
	// 0
	Rifle					UMETA(DisplayName = "Rifle"),
	// 1
	RifleAiming				UMETA(DisplayName = "Rifle Aiming"),
	// 2
	RocketLauncher			UMETA(DisplayName = "Rocket Launcher"),
	// 3
	RocketLauncherAiming	UMETA(DisplayName = "Rocket Launcher Aiming")
};

USTRUCT()
struct GASSHOOTER_API FGSHeroInventory
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<AGSWeapon*> Weapons;

	// Consumable items

	// Passive items like armor

	// Door keys

	// Etc
};

/**
 * A player or AI controlled hero character.
 */
UCLASS()
class GASSHOOTER_API AGSHeroCharacter : public AGSCharacterBase, public IGSInteractable
{
	GENERATED_BODY()
	
public:
	AGSHeroCharacter(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|GSHeroCharacter")
	bool bStartInFirstPersonPerspective;

	FGameplayTag CurrentWeaponTag;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Only called on the Server. Calls before Server's AcknowledgePossession.
	virtual void PossessedBy(AController* NewController) override;

	class UGSFloatingStatusBarWidget* GetFloatingStatusBar();

	// Server handles knockdown - cancel abilities, remove effects, activate knockdown ability
	virtual void KnockDown();

	// Plays knockdown effects for all clients from KnockedDown tag listener on PlayerState
	virtual void PlayKnockDownEffects();

	// Plays revive effects for all clients from KnockedDown tag listener on PlayerState
	virtual void PlayReviveEffects();

	virtual void FinishDying() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter")
	USkeletalMeshComponent* GetFirstPersonMesh() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter")
	USkeletalMeshComponent* GetThirdPersonMesh() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	AGSWeapon* GetCurrentWeapon() const;

	// Adds a new weapon to the inventory.
	// Returns false if the weapon already exists in the inventory, true if it's a new weapon.
	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	bool AddWeaponToInventory(AGSWeapon* NewWeapon, bool bEquipWeapon = false);

	// Removes a weapon from the inventory.
	// Returns true if the weapon exists and was removed. False if the weapon didn't exist in the inventory.
	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	bool RemoveWeaponFromInventory(AGSWeapon* WeaponToRemove);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	void RemoveAllWeaponsFromInventory();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	void EquipWeapon(AGSWeapon* NewWeapon);

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AGSWeapon* NewWeapon);
	void ServerEquipWeapon_Implementation(AGSWeapon* NewWeapon);
	bool ServerEquipWeapon_Validate(AGSWeapon* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	virtual void NextWeapon();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	virtual void PreviousWeapon();

	FName GetWeaponAttachPoint();

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetMaxPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetPrimaryReserveAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetMaxSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetSecondaryReserveAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "GASShooter|Inventory")
	int32 GetNumWeapons() const;


	/**
	* Enhanced Inputs
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ConfirmAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CancelAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrimaryFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SecondaryFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AlternateFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* NextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrevWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* TogglePerspectiveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;

	/**
	* Interactable interface
	*/

	/**
	* We can Interact with other heroes when:
	* Knocked Down - to revive them
	*/
	virtual bool IsAvailableForInteraction_Implementation(UPrimitiveComponent* InteractionComponent) const override;

	/**
	* How long to interact with this player:
	* Knocked Down - to revive them
	*/
	virtual float GetInteractionDuration_Implementation(UPrimitiveComponent* InteractionComponent) const override;

	/**
	* Interaction:
	* Knocked Down - activate revive GA (plays animation)
	*/
	virtual void PreInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent) override;

	/**
	* Interaction:
	* Knocked Down - apply revive GE
	*/
	virtual void PostInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent) override;

	/**
	* Should we wait and who should wait to sync before calling PreInteract():
	* Knocked Down - Yes, client. This will sync the local player's Interact Duration Timer with the knocked down player's
	* revive animation. If we had a picking a player up animation, we could play it on the local player in PreInteract().
	*/
	virtual void GetPreInteractSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* InteractionComponent) const override;

	/**
	* Cancel interaction:
	* Knocked Down - cancel revive ability
	*/
	virtual void CancelInteraction_Implementation(UPrimitiveComponent* InteractionComponent) override;

	/**
	* Get the delegate for this Actor canceling interaction:
	* Knocked Down - cancel being revived if killed
	*/
	FSimpleMulticastDelegate* GetTargetCancelInteractionDelegate(UPrimitiveComponent* InteractionComponent) override;

	void SendLocalInputToASC(bool IsPressed, const EGSAbilityInputID AbilityInputID);

	////////////////////////////////////////////// FP Procedural Animation Public Function
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetLocationLagPos() const;

	//UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	//float GetCrouchAlpha() { return CrouchAlpha; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetWalkAnimPos() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FRotator GetWalkAnimRot() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetWalkAnimAlpha() const;

	//UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	//float GetDipAlpha() { return DipAlpha; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetPitchOffsetPos() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetCamRotOffset() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FRotator GetCamRotCurrent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FRotator GetCamRotRate() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FRotator GetInAirTilt() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetInAirOffset() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FVector GetCamOffsetCurrent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetCamAnimAlpha() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetADSAlpha() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetADSAlphaInversed() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetADSAlphaLerp() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FTransform GetSightTransform() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	FTransform GetRelativeHandTransform() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetHasWeaponAlpha() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetCrouchAlpha() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation")
	float GetDipAlpha() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	FVector StartingThirdPersonMeshLocation;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	FVector StartingFirstPersonMeshLocation;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|Abilities")
	float ReviveDuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|Camera")
	float BaseTurnRate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|Camera")
	float BaseLookUpRate;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float StartingThirdPersonCameraBoomArmLength;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	FVector StartingThirdPersonCameraBoomLocation;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|GSHeroCharacter")
	bool bWasInFirstPersonPerspectiveWhenKnockedDown;

	bool bASCInputBound;

	// Set to true when we change the weapon predictively and flip it to false when the Server replicates to confirm.
	// We use this if the Server refused a weapon change ability's activation to ask the Server to sync the client back up
	// with the correct CurrentWeapon.
	bool bChangedWeaponLocally;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float Default1PFOV;

	UPROPERTY(BlueprintReadOnly, Category = "GASShooter|Camera")
	float Default3PFOV;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|GSHeroCharacter")
	FName WeaponAttachPoint;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class USpringArmComponent* ThirdPersonCameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class UCameraComponent* ThirdPersonCamera;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USceneComponent* FP_Root;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USpringArmComponent* Mesh_Root;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USceneComponent* Offset_Root;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USpringArmComponent* Cam_Root = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	USkeletalMeshComponent* Cam_Skel;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|Camera")
	class UCameraComponent* FirstPersonCamera;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSFloatingStatusBarWidget> UIFloatingStatusBarClass;

	UPROPERTY()
	class UGSFloatingStatusBarWidget* UIFloatingStatusBar;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "GASShooter|UI")
	class UWidgetComponent* UIFloatingStatusBarComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	FGSHeroInventory Inventory;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|Inventory")
	TArray<TSubclassOf<AGSWeapon>> DefaultInventoryWeaponClasses;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AGSWeapon* CurrentWeapon;

	UPROPERTY()
	class UGSAmmoAttributeSet* AmmoAttributeSet;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> KnockDownEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> ReviveEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GASShooter|GSHeroCharacter")
	TSubclassOf<UGameplayEffect> DeathEffect;

	FSimpleMulticastDelegate InteractionCanceledDelegate;

	// Cache tags
	FGameplayTag NoWeaponTag;
	FGameplayTag WeaponChangingDelayReplicationTag;
	FGameplayTag WeaponAmmoTypeNoneTag;
	FGameplayTag WeaponAbilityTag;
	FGameplayTag KnockedDownTag;
	FGameplayTag InteractingTag;

	// Attribute changed delegate handles
	FDelegateHandle PrimaryReserveAmmoChangedDelegateHandle;
	FDelegateHandle SecondaryReserveAmmoChangedDelegateHandle;

	// Tag changed delegate handles
	FDelegateHandle WeaponChangingDelayReplicationTagChangedDelegateHandle;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintNativeEvent)
	void CharacterInitialSpawnDefaultInventory();

	// Toggles between perspectives
	void TogglePerspective();

	// Sets the perspective
	void SetPerspective(bool Is1PPerspective);

	// Creates and initializes the floating status bar for heroes.
	// Safe to call many times because it checks to make sure it only executes once.
	UFUNCTION()
	void InitializeFloatingStatusBar();

	// Client only
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

	// Called from both SetupPlayerInputComponent and OnRep_PlayerState because of a potential race condition where the PlayerController might
	// call ClientRestart which calls SetupPlayerInputComponent before the PlayerState is repped to the client so the PlayerState would be null in SetupPlayerInputComponent.
	// Conversely, the PlayerState might be repped before the PlayerController calls ClientRestart so the Actor's InputComponent would be null in OnRep_PlayerState.
	void BindASCInput();

	// Server spawns default inventory
	UFUNCTION(BlueprintCallable)
	void SpawnDefaultInventory();

	void SetupStartupPerspective();

	bool DoesWeaponExistInInventory(AGSWeapon* InWeapon);

	void SetCurrentWeapon(AGSWeapon* NewWeapon, AGSWeapon* LastWeapon);

	// Unequips the specified weapon. Used when OnRep_CurrentWeapon fires.
	void UnEquipWeapon(AGSWeapon* WeaponToUnEquip);

	// Unequips the current weapon. Used if for example we drop the current weapon.
	void UnEquipCurrentWeapon();

	void InvokeAbility(const FInputActionValue& Value, EGSAbilityInputID Id, bool IsActive);

	UFUNCTION()
	virtual void CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo);

	UFUNCTION()
	virtual void CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo);

	// Attribute changed callbacks
	virtual void CurrentWeaponPrimaryReserveAmmoChanged(const FOnAttributeChangeData& Data);
	virtual void CurrentWeaponSecondaryReserveAmmoChanged(const FOnAttributeChangeData& Data);

	// Tag changed callbacks
	virtual void WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void OnRep_CurrentWeapon(AGSWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_Inventory();

	void OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags);
	
	// The CurrentWeapon is only automatically replicated to simulated clients.
	// The autonomous client can use this to request the proper CurrentWeapon from the server when it knows it may be
	// out of sync with it from predictive client-side changes.
	UFUNCTION(Server, Reliable)
	void ServerSyncCurrentWeapon();
	void ServerSyncCurrentWeapon_Implementation();
	bool ServerSyncCurrentWeapon_Validate();
	
	// The CurrentWeapon is only automatically replicated to simulated clients.
	// Use this function to manually sync the autonomous client's CurrentWeapon when we're ready to.
	// This allows us to predict weapon changes (changing weapons fast multiple times in a row so that the server doesn't
	// replicate and clobber our CurrentWeapon).
	UFUNCTION(Client, Reliable)
	void ClientSyncCurrentWeapon(AGSWeapon* InWeapon);
	void ClientSyncCurrentWeapon_Implementation(AGSWeapon* InWeapon);
	bool ClientSyncCurrentWeapon_Validate(AGSWeapon* InWeapon);

	//////////////////////////////////////////////////////////////////// FP Procedural Animation Privates
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	float BaseWalkSpeed{ 600.f };

	UTimelineComponent* WalkingTL = nullptr;

	UCurveFloat* WalkLeftRightAlphaCurve = nullptr;
	UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	void WalkLeftRightTLCallback(float val);
	float WalkLeftRightAlpha;

	UCurveFloat* WalkFwdBwdAlphaCurve = nullptr;
	UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	void WalkFwdBwdTLCallback(float val);
	float WalkFwdBwdAlpha;

	UCurveFloat* WalkRollAlphaCurve = nullptr;
	UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	void WalkRollTLCallback(float val);
	float WalkRollAlpha;

	//UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	//void WalkTLFootstepCallback();

	UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Walk")
	void WalkTLUpdateEvent();

	FVector WalkAnimPos;
	FRotator WalkAnimRot;
	float WalkAnimAlpha;
	FVector LocationLagPos;
	void UpdateVelocityVars();

	FVector PitchOffsetPos;
	FVector CamRotOffset;
	FRotator CamRotCurrent;
	FRotator CamRotRate;
	FVector InAirOffset;
	FRotator InAirTilt;
	void UpdateLookInputVars(FRotator CamRotPrev);

	void ProcCamAnim(FVector& CamOffsetArg, float& CamAnimAlphaArg);
	FVector PrevHandLoc;
	FVector CamOffset;
	float CamStrength{ 25.f };
	FVector CamOffsetCurrent;
	float CamAnimAlpha{ 0.f };

	/// ADS
	UPROPERTY(BlueprintReadonly, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|ADS")
	UTimelineComponent* ADSTL = nullptr;

	//UCurveFloat* ADSAlphaCurve = nullptr;
	//UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|ADS")
	//void ADSTLCallback(float val);

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|ADS")
	float ADSAlpha{ 0.f };

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|ADS")
	float ADSAlphaInversed{ 1.f }; // basically 1 - ADSAlpha

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|ADS")
	float ADSAlphaLerp{ 1.f }; // Same thing as ADSAlphaInversed, but clamped between 0.2f - 1.f

	FTransform SightTransform;
	FTransform RelativeHandTransform;

	/// Crouch
	//virtual void RecalculateBaseEyeHeight() override;
	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	UPROPERTY(BlueprintReadonly, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	UTimelineComponent* CrouchTL = nullptr;

	UCurveFloat* CrouchAlphaCurve = nullptr;

	UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	void CrouchTLCallback(float val);

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	float CrouchAlpha{ 0.f };

	UPROPERTY(BlueprintReadonly, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	FTimerHandle UnCrouchTimerHandle;

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	float StandHeight{ 96.f };

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Crouch")
	float CrouchHeight{ 68.f };

	//float TargetHalfHeight{ 96.f };

	/// Jump
	int32 JumpsLeft{ 2 };
	int32 JumpsMax{ 2 };

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;
	virtual bool CanJumpInternal_Implementation() const override;

	FTimerHandle CoyoteTimerHandle;
	void CoyoteTimePassed();
	float CoyoteTime{ 0.35f };

	UFUNCTION(BlueprintImplementableEvent, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Jump")
	void Dip(float Speed = 1.f, float Strength = 1.f);

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Jump")
	float DipStrength{ 1.f };

	//UPROPERTY(BlueprintReadonly, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Jump")
	//UTimelineComponent* DipTL = nullptr;

	//UCurveFloat* DipAlphaCurve = nullptr;

	//UFUNCTION(Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Jump")
	//void DipTlCallback(float val);

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|GSHeroCharacter|Procedural FP Animation|Jump")
	float DipAlpha;

	void LandingDip();
};
