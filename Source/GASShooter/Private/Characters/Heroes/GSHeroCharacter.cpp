// Copyright 2020 Dan Kestranek.


#include "Characters/Heroes/GSHeroCharacter.h"
#include "Animation/AnimInstance.h"
#include "AI/GSHeroAIController.h"
#include "Camera/CameraComponent.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Abilities/GSAbilitySystemGlobals.h"
#include "Characters/Abilities/AttributeSets/GSAmmoAttributeSet.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GASShooter/GASShooterGameModeBase.h"
#include "GSBlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/GSPlayerController.h"
#include "Player/GSPlayerState.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "UI/GSFloatingStatusBarWidget.h"
#include "Weapons/GSWeapon.h"
#include "EnhancedInputSubsystems.h"
#include "Components/TimelineComponent.h"

AGSHeroCharacter::AGSHeroCharacter(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;
	bStartInFirstPersonPerspective = true;
	bIsFirstPersonPerspective = false;
	bWasInFirstPersonPerspectiveWhenKnockedDown = false;
	bASCInputBound = false;
	bChangedWeaponLocally = false;
	Default1PFOV = 90.0f;
	Default3PFOV = 80.0f;
	NoWeaponTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Equipped.None"));
	WeaponChangingDelayReplicationTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.IsChangingDelayReplication"));
	WeaponAmmoTypeNoneTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));
	WeaponAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon"));
	CurrentWeaponTag = NoWeaponTag;
	Inventory = FGSHeroInventory();
	ReviveDuration = 4.0f;

	GetCapsuleComponent()->InitCapsuleSize(35.f, 96.f);

	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 3072.f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f;
	GetCharacterMovement()->PerchRadiusThreshold = 30.f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->JumpZVelocity = 750.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 200.f;
	GetCharacterMovement()->AirControl = 0.275f;
	
	ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(FName("CameraBoom"));
	ThirdPersonCameraBoom->SetupAttachment(RootComponent);
	ThirdPersonCameraBoom->bUsePawnControlRotation = true;
	ThirdPersonCameraBoom->SetRelativeLocation(FVector(0, 50, 68.492264));

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(FName("FollowCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom);
	ThirdPersonCamera->FieldOfView = Default3PFOV;

	FP_Root = CreateDefaultSubobject<USceneComponent>(TEXT("FP_Root"));
	FP_Root->SetupAttachment(RootComponent);

	FirstPersonLegMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("FirstPersonLegMesh"));
	FirstPersonLegMesh->SetupAttachment(FP_Root);
	FirstPersonLegMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonLegMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonLegMesh->bReceivesDecals = false;
	FirstPersonLegMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	FirstPersonLegMesh->CastShadow = false;
	FirstPersonLegMesh->SetVisibility(false, true);
	FirstPersonLegMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonLegMesh->SetRelativeLocation(FVector(0.f, 0.f, -79.5f));

	Mesh_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Mesh_Root"));
	Mesh_Root->SetupAttachment(FP_Root);
	Mesh_Root->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Mesh_Root->TargetArmLength = 0;
	Mesh_Root->bDoCollisionTest = false;
	Mesh_Root->bUsePawnControlRotation = true;
	Mesh_Root->bInheritPitch = true;
	Mesh_Root->bInheritYaw = true;
	Mesh_Root->bInheritRoll = false;

	Offset_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	Offset_Root->SetupAttachment(Mesh_Root);
	Offset_Root->SetRelativeLocation(FVector(0.f, 0.f, -70.f));

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(Offset_Root);
	FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->bReceivesDecals = false;
	FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetVisibility(false, true);
	FirstPersonMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonMesh->SetRelativeLocation(FVector(0.f, 0.f, -79.5f));

	Cam_Root = CreateDefaultSubobject<USpringArmComponent>(TEXT("Cam_Root"));
	Cam_Root->SetupAttachment(FP_Root);
	Cam_Root->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Cam_Root->TargetArmLength = 0;
	Cam_Root->bDoCollisionTest = false;
	Cam_Root->bUsePawnControlRotation = true;
	Cam_Root->bInheritPitch = true;
	Cam_Root->bInheritYaw = true;
	Cam_Root->bInheritRoll = false;

	Cam_Skel = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Cam_Skel"));
	Cam_Skel->SetupAttachment(Cam_Root);
	Cam_Skel->SetRelativeLocation(FVector(0.f, 0.f, -70.f));

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(FName("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(Cam_Skel);
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->PostProcessSettings.bOverride_VignetteIntensity = true;

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(FName("NoCollision"));
	GetMesh()->SetCollisionResponseToChannel(COLLISION_INTERACTABLE, ECollisionResponse::ECR_Overlap);
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->bReceivesDecals = false;

	UIFloatingStatusBarComponent = CreateDefaultSubobject<UWidgetComponent>(FName("UIFloatingStatusBarComponent"));
	UIFloatingStatusBarComponent->SetupAttachment(RootComponent);
	UIFloatingStatusBarComponent->SetRelativeLocation(FVector(0, 0, 120));
	UIFloatingStatusBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	UIFloatingStatusBarComponent->SetDrawSize(FVector2D(500, 500));

	UIFloatingStatusBarClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/UI/UI_FloatingStatusBar_Hero.UI_FloatingStatusBar_Hero_C"));
	if (!UIFloatingStatusBarClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find UIFloatingStatusBarClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}

	AutoPossessAI = EAutoPossessAI::PlacedInWorld;
	AIControllerClass = AGSHeroAIController::StaticClass();

	// Cache tags
	KnockedDownTag = FGameplayTag::RequestGameplayTag("State.KnockedDown");
	InteractingTag = FGameplayTag::RequestGameplayTag("State.Interacting");


	////////////////////// FP Procedural animation setup
	//CrouchTL = CreateDefaultSubobject<UTimelineComponent>(FName("CrouchTL"));
	//CrouchTL->SetTimelineLength(0.2f);
	//CrouchTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	//FOnTimelineFloat onCrouchTLCallback;
	//onCrouchTLCallback.BindUFunction(this, FName{ TEXT("CrouchTLCallback") });
	//CrouchAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("CrouchAlphaCurve"));
	//CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(CrouchAlphaCurve->FloatCurve.AddKey(0.f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	//CrouchAlphaCurve->FloatCurve.SetKeyInterpMode(CrouchAlphaCurve->FloatCurve.AddKey(0.2f, 1.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	//CrouchTL->AddInterpFloat(CrouchAlphaCurve, onCrouchTLCallback);

	WalkingTL = CreateDefaultSubobject<UTimelineComponent>(FName("WalkingTL"));
	WalkingTL->SetTimelineLength(1.f);
	WalkingTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
	WalkingTL->SetLooping(true);

	FOnTimelineFloat onWalkingLeftRightTLCallback;
	onWalkingLeftRightTLCallback.BindUFunction(this, FName{ TEXT("WalkLeftRightTLCallback") });
	WalkLeftRightAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkLeftRightAlphaCurve"));
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.25f, 0.5f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.5f, 1.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(WalkLeftRightAlphaCurve->FloatCurve.AddKey(0.75f, 0.5f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkLeftRightAlphaCurve->FloatCurve.SetKeyInterpMode(WalkLeftRightAlphaCurve->FloatCurve.AddKey(1.f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkLeftRightAlphaCurve, onWalkingLeftRightTLCallback);

	FOnTimelineFloat onWalkingFwdBwdTLCallback;
	onWalkingFwdBwdTLCallback.BindUFunction(this, FName{ TEXT("WalkFwdBwdTLCallback") });
	WalkFwdBwdAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkFwdBwdAlphaCurve"));
	WalkFwdBwdAlphaCurve->FloatCurve.SetKeyInterpMode(WalkFwdBwdAlphaCurve->FloatCurve.AddKey(0.f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkFwdBwdAlphaCurve->FloatCurve.SetKeyInterpMode(WalkFwdBwdAlphaCurve->FloatCurve.AddKey(0.3f, 1.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkFwdBwdAlphaCurve->FloatCurve.SetKeyInterpMode(WalkFwdBwdAlphaCurve->FloatCurve.AddKey(0.5f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkFwdBwdAlphaCurve->FloatCurve.SetKeyInterpMode(WalkFwdBwdAlphaCurve->FloatCurve.AddKey(0.8f, 1.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkFwdBwdAlphaCurve->FloatCurve.SetKeyInterpMode(WalkFwdBwdAlphaCurve->FloatCurve.AddKey(1.f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkFwdBwdAlphaCurve, onWalkingFwdBwdTLCallback);

	FOnTimelineFloat onWalkingRollTLCallback;
	onWalkingRollTLCallback.BindUFunction(this, FName{ TEXT("WalkRollTLCallback") });
	WalkRollAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("WalkRollAlphaCurve"));
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(0.f, 0.18f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(0.15f, 0.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(0.4f, 0.5f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(0.65f, 1.f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(0.9f, 0.5f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkRollAlphaCurve->FloatCurve.SetKeyInterpMode(WalkRollAlphaCurve->FloatCurve.AddKey(1.f, 0.18f), ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	WalkingTL->AddInterpFloat(WalkRollAlphaCurve, onWalkingRollTLCallback);

	/*FOnTimelineEvent footstepEvent;
	footstepEvent.BindUFunction(this, FName{ TEXT("WalkTLFootstepCallback") });
	WalkingTL->AddEvent(0.35f, footstepEvent);
	WalkingTL->AddEvent(0.85f, footstepEvent);*/

	FOnTimelineEvent updateWalkEvent;
	updateWalkEvent.BindUFunction(this, FName{ TEXT("WalkTLUpdateEvent") });
	WalkingTL->SetTimelinePostUpdateFunc(updateWalkEvent);

	//MoveMode = ECustomMovementMode::Walking;

	//SlideTL = CreateDefaultSubobject<UTimelineComponent>(FName("SlideTL"));
	//SlideTL->SetTimelineLength(1.f);
	//SlideTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	//FOnTimelineFloat onSlideTLCallback;
	//onSlideTLCallback.BindUFunction(this, FName{ TEXT("SlideTLCallback") });
	//SlideAlphaCurve = CreateDefaultSubobject<UCurveFloat>(FName("SlideAlphaCurve"));
	//KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(0.f, 1.f);
	//SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	//KeyHandle = SlideAlphaCurve->FloatCurve.AddKey(1.f, 0.f);
	//SlideAlphaCurve->FloatCurve.SetKeyInterpMode(KeyHandle, ERichCurveInterpMode::RCIM_Cubic, /*auto*/true);
	//SlideTL->AddInterpFloat(SlideAlphaCurve, onSlideTLCallback);
	//FOnTimelineEvent onSlideTLFinished;
	//onSlideTLFinished.BindUFunction(this, FName{ TEXT("FinishedSlideDelegate") });
	//SlideTL->SetTimelineFinishedFunc(onSlideTLFinished);

	ADSTL = CreateDefaultSubobject<UTimelineComponent>(FName("ADSTL"));
	ADSTL->SetTimelineLength(1.f);
	ADSTL->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
}

void AGSHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSHeroCharacter, Inventory);
	// Only replicate CurrentWeapon to simulated clients and manually sync CurrentWeeapon with Owner when we're ready.
	// This allows us to predict weapon changing.
	DOREPLIFETIME_CONDITION(AGSHeroCharacter, CurrentWeapon, COND_SimulatedOnly);
}

// Called to bind functionality to input
void AGSHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(PrimaryFireAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::PrimaryFire, true);

		EnhancedInputComponent->BindAction(PrimaryFireAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::PrimaryFire, false);

		EnhancedInputComponent->BindAction(SecondaryFireAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::SecondaryFire, true);

		EnhancedInputComponent->BindAction(SecondaryFireAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::SecondaryFire, false);

		EnhancedInputComponent->BindAction(AlternateFireAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::AlternateFire, true);

		EnhancedInputComponent->BindAction(AlternateFireAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::AlternateFire, false);

		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Reload, true);

		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Reload, false);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Sprint, true);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Sprint, false);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Crouch, true);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Crouch, false);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Jump, true);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Jump, false);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Interact, true);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Interact, false);

		EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Confirm, true);

		EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Confirm, false);

		EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Cancel, true);

		EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::Cancel, false);

		EnhancedInputComponent->BindAction(NextWeaponAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::NextWeapon, true);

		EnhancedInputComponent->BindAction(NextWeaponAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::NextWeapon, false);

		EnhancedInputComponent->BindAction(PrevWeaponAction, ETriggerEvent::Started, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::PrevWeapon, true);

		EnhancedInputComponent->BindAction(PrevWeaponAction, ETriggerEvent::Completed, this,
			&AGSHeroCharacter::InvokeAbility, EGSAbilityInputID::PrevWeapon, false);

		EnhancedInputComponent->BindAction(TogglePerspectiveAction, ETriggerEvent::Started, this, &AGSHeroCharacter::TogglePerspective);
	}

	// Bind player input to the AbilitySystemComponent. Also called in OnRep_PlayerState because of a potential race condition.
	BindASCInput();
}

// Server only
void AGSHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
		AbilitySystemComponent = Cast<UGSAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		// AI won't have PlayerControllers so we can init again here just to be sure. No harm in initing twice for heroes that have PlayerControllers.
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

		WeaponChangingDelayReplicationTagChangedDelegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(WeaponChangingDelayReplicationTag)
			.AddUObject(this, &AGSHeroCharacter::WeaponChangingDelayReplicationTagChanged);

		// Set the AttributeSetBase for convenience attribute functions
		AttributeSetBase = PS->GetAttributeSetBase();

		AmmoAttributeSet = PS->GetAmmoAttributeSet();

		// If we handle players disconnecting and rejoining in the future, we'll have to change this so that possession from rejoining doesn't reset attributes.
		// For now assume possession = spawn/respawn.
		InitializeAttributes();

		AddStartupEffects();

		AddCharacterAbilities();

		AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
		if (PC)
		{
			PC->CreateHUD();

			WalkingTL->Play();
			UE_LOG(LogTemp, Log, TEXT("Play walkingTL from posessed by"));
		}

		if (AbilitySystemComponent->GetTagCount(DeadTag) > 0)
		{
			// Set Health/Mana/Stamina to their max. This is only necessary for *Respawn*.
			SetHealth(GetMaxHealth());
			SetMana(GetMaxMana());
			SetStamina(GetMaxStamina());
			SetShield(GetMaxShield());
		}

		// Remove Dead tag
		AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(DeadTag));

		InitializeFloatingStatusBar();

		// If player is host on listen server, the floating status bar would have been created for them from BeginPlay before player possession, hide it
		if (IsLocallyControlled() && IsPlayerControlled() && UIFloatingStatusBarComponent && UIFloatingStatusBar)
		{
			UIFloatingStatusBarComponent->SetVisibility(false, true);
		}
	}

	SetupStartupPerspective();
}

UGSFloatingStatusBarWidget* AGSHeroCharacter::GetFloatingStatusBar()
{
	return UIFloatingStatusBar;
}

void AGSHeroCharacter::KnockDown()
{
	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->ApplyGameplayEffectToSelf(Cast<UGameplayEffect>(KnockDownEffect->GetDefaultObject()), 1.0f, AbilitySystemComponent->MakeEffectContext());
	}

	SetHealth(GetMaxHealth());
	SetShield(0.0f);
}

void AGSHeroCharacter::PlayKnockDownEffects()
{
	// Store perspective to restore on Revive
	bWasInFirstPersonPerspectiveWhenKnockedDown = IsInFirstPersonPerspective();

	SetPerspective(false);

	// Play it here instead of in the ability to skip extra replication data
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	if (AbilitySystemComponent)
	{
		FGameplayCueParameters GCParameters;
		GCParameters.Location = GetActorLocation();
		AbilitySystemComponent->ExecuteGameplayCueLocal(FGameplayTag::RequestGameplayTag("GameplayCue.Hero.KnockedDown"), GCParameters);
	}
}

void AGSHeroCharacter::PlayReviveEffects()
{
	// Restore perspective the player had when knocked down
	SetPerspective(bWasInFirstPersonPerspectiveWhenKnockedDown);

	// Play revive particles or sounds here (we don't have any)
	if (AbilitySystemComponent)
	{
		FGameplayCueParameters GCParameters;
		GCParameters.Location = GetActorLocation();
		AbilitySystemComponent->ExecuteGameplayCueLocal(FGameplayTag::RequestGameplayTag("GameplayCue.Hero.Revived"), GCParameters);
	}
}

void AGSHeroCharacter::FinishDying()
{
	// AGSHeroCharacter doesn't follow AGSCharacterBase's pattern of Die->Anim->FinishDying because AGSHeroCharacter can be knocked down
	// to either be revived, bleed out, or finished off by an enemy.

	if (!HasAuthority())
	{
		return;
	}

	RemoveAllWeaponsFromInventory();

	AbilitySystemComponent->RegisterGameplayTagEvent(WeaponChangingDelayReplicationTag).Remove(WeaponChangingDelayReplicationTagChangedDelegateHandle);

	AGASShooterGameModeBase* GM = Cast<AGASShooterGameModeBase>(GetWorld()->GetAuthGameMode());

	if (GM)
	{
		GM->HeroDied(GetController());
	}

	RemoveCharacterAbilities();

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->ApplyGameplayEffectToSelf(Cast<UGameplayEffect>(DeathEffect->GetDefaultObject()), 1.0f, AbilitySystemComponent->MakeEffectContext());
	}

	OnCharacterDied.Broadcast(this);

	Super::FinishDying();
}

USkeletalMeshComponent* AGSHeroCharacter::GetFirstPersonMesh() const
{
	return FirstPersonMesh;
}

USkeletalMeshComponent* AGSHeroCharacter::GetThirdPersonMesh() const
{
	return GetMesh();
}

AGSWeapon* AGSHeroCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

bool AGSHeroCharacter::AddWeaponToInventory(AGSWeapon* NewWeapon, bool bEquipWeapon)
{
	if (DoesWeaponExistInInventory(NewWeapon))
	{
		USoundCue* PickupSound = NewWeapon->GetPickupSound();

		if (PickupSound && IsLocallyControlled())
		{
			UGameplayStatics::SpawnSoundAttached(PickupSound, GetRootComponent());
		}

		if (GetLocalRole() < ROLE_Authority)
		{
			return false;
		}

		// Create a dynamic instant Gameplay Effect to give the primary and secondary ammo
		UGameplayEffect* GEAmmo = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Ammo")));
		GEAmmo->DurationPolicy = EGameplayEffectDurationType::Instant;

		if (NewWeapon->PrimaryAmmoType != WeaponAmmoTypeNoneTag)
		{
			int32 Idx = GEAmmo->Modifiers.Num();
			GEAmmo->Modifiers.SetNum(Idx + 1);

			FGameplayModifierInfo& InfoPrimaryAmmo = GEAmmo->Modifiers[Idx];
			InfoPrimaryAmmo.ModifierMagnitude = FScalableFloat(NewWeapon->GetPrimaryClipAmmo());
			InfoPrimaryAmmo.ModifierOp = EGameplayModOp::Additive;
			InfoPrimaryAmmo.Attribute = UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(NewWeapon->PrimaryAmmoType);
		}

		if (NewWeapon->SecondaryAmmoType != WeaponAmmoTypeNoneTag)
		{
			int32 Idx = GEAmmo->Modifiers.Num();
			GEAmmo->Modifiers.SetNum(Idx + 1);

			FGameplayModifierInfo& InfoSecondaryAmmo = GEAmmo->Modifiers[Idx];
			InfoSecondaryAmmo.ModifierMagnitude = FScalableFloat(NewWeapon->GetSecondaryClipAmmo());
			InfoSecondaryAmmo.ModifierOp = EGameplayModOp::Additive;
			InfoSecondaryAmmo.Attribute = UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(NewWeapon->SecondaryAmmoType);
		}

		if (GEAmmo->Modifiers.Num() > 0)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(GEAmmo, 1.0f, AbilitySystemComponent->MakeEffectContext());
		}

		NewWeapon->Destroy();

		return false;
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		return false;
	}

	Inventory.Weapons.Add(NewWeapon);
	NewWeapon->SetOwningCharacter(this);
	NewWeapon->AddAbilities();

	if (bEquipWeapon)
	{
		EquipWeapon(NewWeapon);
		ClientSyncCurrentWeapon(CurrentWeapon);
	}

	return true;
}

bool AGSHeroCharacter::RemoveWeaponFromInventory(AGSWeapon* WeaponToRemove)
{
	if (DoesWeaponExistInInventory(WeaponToRemove))
	{
		if (WeaponToRemove == CurrentWeapon)
		{
			UnEquipCurrentWeapon();
		}

		Inventory.Weapons.Remove(WeaponToRemove);
		WeaponToRemove->RemoveAbilities();
		WeaponToRemove->SetOwningCharacter(nullptr);
		WeaponToRemove->ResetWeapon();

		// Add parameter to drop weapon?

		return true;
	}

	return false;
}

void AGSHeroCharacter::RemoveAllWeaponsFromInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	UnEquipCurrentWeapon();

	float radius = 50.0f;
	float NumWeapons = Inventory.Weapons.Num();

	for (int32 i = Inventory.Weapons.Num() - 1; i >= 0; i--)
	{
		AGSWeapon* Weapon = Inventory.Weapons[i];
		RemoveWeaponFromInventory(Weapon);

		// Set the weapon up as a pickup

		float OffsetX = radius * FMath::Cos((i / NumWeapons) * 2.0f * PI);
		float OffsetY = radius * FMath::Sin((i / NumWeapons) * 2.0f * PI);
		Weapon->OnDropped(GetActorLocation() + FVector(OffsetX, OffsetY, 0.0f));
	}
}

void AGSHeroCharacter::EquipWeapon(AGSWeapon* NewWeapon)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEquipWeapon(NewWeapon);
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
		bChangedWeaponLocally = true;
	}
	else
	{
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
	}
}

void AGSHeroCharacter::ServerEquipWeapon_Implementation(AGSWeapon* NewWeapon)
{
	EquipWeapon(NewWeapon);
}

bool AGSHeroCharacter::ServerEquipWeapon_Validate(AGSWeapon* NewWeapon)
{
	return true;
}

void AGSHeroCharacter::NextWeapon()
{
	if (Inventory.Weapons.Num() < 2)
	{
		return;
	}

	int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);
	UnEquipCurrentWeapon();

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		EquipWeapon(Inventory.Weapons[0]);
	}
	else
	{
		EquipWeapon(Inventory.Weapons[(CurrentWeaponIndex + 1) % Inventory.Weapons.Num()]);
	}
}

void AGSHeroCharacter::PreviousWeapon()
{
	if (Inventory.Weapons.Num() < 2)
	{
		return;
	}

	int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);

	UnEquipCurrentWeapon();

	if (CurrentWeaponIndex == INDEX_NONE)
	{
		EquipWeapon(Inventory.Weapons[0]);
	}
	else
	{
		int32 IndexOfPrevWeapon = FMath::Abs(CurrentWeaponIndex - 1 + Inventory.Weapons.Num()) % Inventory.Weapons.Num();
		EquipWeapon(Inventory.Weapons[IndexOfPrevWeapon]);
	}
}

FName AGSHeroCharacter::GetWeaponAttachPoint()
{
	return WeaponAttachPoint;
}

int32 AGSHeroCharacter::GetPrimaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetPrimaryClipAmmo();
	}

	return 0;
}

int32 AGSHeroCharacter::GetMaxPrimaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetMaxPrimaryClipAmmo();
	}

	return 0;
}

int32 AGSHeroCharacter::GetPrimaryReserveAmmo() const
{
	if (CurrentWeapon && AmmoAttributeSet)
	{
		FGameplayAttribute Attribute = AmmoAttributeSet->GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType);
		if (Attribute.IsValid())
		{
			return AbilitySystemComponent->GetNumericAttribute(Attribute);
		}
	}

	return 0;
}

int32 AGSHeroCharacter::GetSecondaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetSecondaryClipAmmo();
	}

	return 0;
}

int32 AGSHeroCharacter::GetMaxSecondaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetMaxSecondaryClipAmmo();
	}

	return 0;
}

int32 AGSHeroCharacter::GetSecondaryReserveAmmo() const
{
	if (CurrentWeapon)
	{
		FGameplayAttribute Attribute = AmmoAttributeSet->GetReserveAmmoAttributeFromTag(CurrentWeapon->SecondaryAmmoType);
		if (Attribute.IsValid())
		{
			return AbilitySystemComponent->GetNumericAttribute(Attribute);
		}
	}

	return 0;
}

int32 AGSHeroCharacter::GetNumWeapons() const
{
	return Inventory.Weapons.Num();
}

bool AGSHeroCharacter::IsAvailableForInteraction_Implementation(UPrimitiveComponent* InteractionComponent) const
{
	// Hero is available to be revived if knocked down and is not already being revived.
	// If you want multiple heroes reviving someone to speed it up, you would need to change GA_Interact
	// (outside the scope of this sample).
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(InteractingTag))
	{
		return true;
	}
	
	return IGSInteractable::IsAvailableForInteraction_Implementation(InteractionComponent);
}

float AGSHeroCharacter::GetInteractionDuration_Implementation(UPrimitiveComponent* InteractionComponent) const
{
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag))
	{
		return ReviveDuration;
	}

	return IGSInteractable::GetInteractionDuration_Implementation(InteractionComponent);
}

void AGSHeroCharacter::PreInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent)
{
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag) && HasAuthority())
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Revive")));
	}
}

void AGSHeroCharacter::PostInteract_Implementation(AActor* InteractingActor, UPrimitiveComponent* InteractionComponent)
{
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag) && HasAuthority())
	{
		AbilitySystemComponent->ApplyGameplayEffectToSelf(Cast<UGameplayEffect>(ReviveEffect->GetDefaultObject()), 1.0f, AbilitySystemComponent->MakeEffectContext());
	}
}

void AGSHeroCharacter::GetPreInteractSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* InteractionComponent) const
{
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag))
	{
		bShouldSync = true;
		Type = EAbilityTaskNetSyncType::OnlyClientWait;
		return;
	}

	IGSInteractable::GetPreInteractSyncType_Implementation(bShouldSync, Type, InteractionComponent);
}

void AGSHeroCharacter::CancelInteraction_Implementation(UPrimitiveComponent* InteractionComponent)
{
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag) && HasAuthority())
	{
		FGameplayTagContainer CancelTags(FGameplayTag::RequestGameplayTag("Ability.Revive"));
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}
}

FSimpleMulticastDelegate* AGSHeroCharacter::GetTargetCancelInteractionDelegate(UPrimitiveComponent* InteractionComponent)
{
	return &InteractionCanceledDelegate;
}

void AGSHeroCharacter::SendLocalInputToASC(bool IsPressed, const EGSAbilityInputID AbilityInputID)
{
	if (AbilitySystemComponent)
	{
		if (IsPressed)
		{
			AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(AbilityInputID));
		}
		else
		{
			AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(AbilityInputID));
		}
	}
}

/**
* On the Server, Possession happens before BeginPlay.
* On the Client, BeginPlay happens before Possession.
* So we can't use BeginPlay to do anything with the AbilitySystemComponent because we don't have it until the PlayerState replicates from possession.
*/
void AGSHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	StartingFirstPersonMeshLocation = FirstPersonMesh->GetRelativeLocation();

	// Only needed for Heroes placed in world and when the player is the Server.
	// On respawn, they are set up in PossessedBy.
	// When the player a client, the floating status bars are all set up in OnRep_PlayerState.
	InitializeFloatingStatusBar();

	// CurrentWeapon is replicated only to Simulated clients so sync the current weapon manually
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSyncCurrentWeapon();
	}
	
	//if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	//{
	//	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	//	{
	//		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	//	}
	//}
}

void AGSHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cancel being revived if killed
	//InteractionCanceledDelegate.Broadcast();
	Execute_InteractableCancelInteraction(this, GetThirdPersonMesh());

	// Clear CurrentWeaponTag on the ASC. This happens naturally in UnEquipCurrentWeapon() but
	// that is only called on the server from hero death (the OnRep_CurrentWeapon() would have
	// handled it on the client but that is never called due to the hero being marked pending
	// destroy). This makes sure the client has it cleared.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		CurrentWeaponTag = NoWeaponTag;
		AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
	}

	Super::EndPlay(EndPlayReason);
}

void AGSHeroCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	StartingThirdPersonCameraBoomArmLength = ThirdPersonCameraBoom->TargetArmLength;
	StartingThirdPersonCameraBoomLocation = ThirdPersonCameraBoom->GetRelativeLocation();
	StartingThirdPersonMeshLocation = GetMesh()->GetRelativeLocation();

	CharacterInitialSpawnDefaultInventory();
}

void AGSHeroCharacter::CharacterInitialSpawnDefaultInventory_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Called from %s with original CharacterInitialSpawnDefaultInventory implementation."), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("%s() original implementation called. Override this in BP instead!."), *FString(__FUNCTION__));
}

void AGSHeroCharacter::TogglePerspective()
{
	// If knocked down, always be in 3rd person
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag))
	{
		return;
	}

	bIsFirstPersonPerspective = !bIsFirstPersonPerspective;
	SetPerspective(bIsFirstPersonPerspective);
}

void AGSHeroCharacter::SetPerspective(bool InIsFirstPersonPerspective)
{
	// If knocked down, always be in 3rd person
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(KnockedDownTag) && InIsFirstPersonPerspective)
	{
		return;
	}

	// Only change perspective for the locally controlled player. Simulated proxies should stay in third person.
	// To swap cameras, deactivate current camera (defaults to ThirdPersonCamera), activate desired camera, and call PlayerController->SetViewTarget() on self
	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalPlayerController())
	{
		if (InIsFirstPersonPerspective)
		{
			ThirdPersonCamera->Deactivate();
			FirstPersonCamera->Activate();
			PC->SetViewTarget(this);

			GetMesh()->SetVisibility(false, true);
			FirstPersonMesh->SetVisibility(true, true);
			FirstPersonLegMesh->SetVisibility(true, true);

			// Move third person mesh back so that the shadow doesn't look disconnected
			GetMesh()->SetRelativeLocation(StartingThirdPersonMeshLocation + FVector(InvisibleBodyMeshOffsetLength, 0.0f, 0.0f));
			FirstPersonLegMesh->SetRelativeLocation(StartingThirdPersonMeshLocation + FVector(InvisibleBodyMeshOffsetLength, 0.0f, 0.0f));
		}
		else
		{
			FirstPersonCamera->Deactivate();
			ThirdPersonCamera->Activate();
			PC->SetViewTarget(this);

			FirstPersonMesh->SetVisibility(false, true);
			FirstPersonLegMesh->SetVisibility(false, true);
			GetMesh()->SetVisibility(true, true);

			// Reset the third person mesh
			GetMesh()->SetRelativeLocation(StartingThirdPersonMeshLocation);
			FirstPersonLegMesh->SetRelativeLocation(StartingThirdPersonMeshLocation);
		}
	}
}

void AGSHeroCharacter::InitializeFloatingStatusBar()
{
	// Only create once
	if (UIFloatingStatusBar || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	// Don't create for locally controlled player. We could add a game setting to toggle this later.
	if (IsPlayerControlled() && IsLocallyControlled())
	{
		return;
	}

	// Need a valid PlayerState
	if (!GetPlayerState())
	{
		return;
	}

	// Setup UI for Locally Owned Players only, not AI or the server's copy of the PlayerControllers
	AGSPlayerController* PC = Cast<AGSPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC && PC->IsLocalPlayerController())
	{
		if (UIFloatingStatusBarClass)
		{
			UIFloatingStatusBar = CreateWidget<UGSFloatingStatusBarWidget>(PC, UIFloatingStatusBarClass);
			if (UIFloatingStatusBar && UIFloatingStatusBarComponent)
			{
				UIFloatingStatusBarComponent->SetWidget(UIFloatingStatusBar);

				// Setup the floating status bar
				UIFloatingStatusBar->SetHealthPercentage(GetHealth() / GetMaxHealth());
				UIFloatingStatusBar->SetManaPercentage(GetMana() / GetMaxMana());
				UIFloatingStatusBar->SetShieldPercentage(GetShield() / GetMaxShield());
				UIFloatingStatusBar->OwningCharacter = this;
				UIFloatingStatusBar->SetCharacterName(CharacterName);
			}
		}
	}
}

// Client only
void AGSHeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		// Set the ASC for clients. Server does this in PossessedBy.
		AbilitySystemComponent = Cast<UGSAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		// Init ASC Actor Info for clients. Server will init its ASC when it possesses a new Actor.
		AbilitySystemComponent->InitAbilityActorInfo(PS, this);

		// Bind player input to the AbilitySystemComponent. Also called in SetupPlayerInputComponent because of a potential race condition.
		BindASCInput();

		AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &AGSHeroCharacter::OnAbilityActivationFailed);

		// Set the AttributeSetBase for convenience attribute functions
		AttributeSetBase = PS->GetAttributeSetBase();
		
		AmmoAttributeSet = PS->GetAmmoAttributeSet();

		// If we handle players disconnecting and rejoining in the future, we'll have to change this so that posession from rejoining doesn't reset attributes.
		// For now assume possession = spawn/respawn.
		InitializeAttributes();

		AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
		if (PC)
		{
			PC->CreateHUD();

			// setup FP animation if controlled by player (client only)
			WalkingTL->Play();
			UE_LOG(LogTemp, Log, TEXT("Play walkingTL from onrep_playerstate"));
		}
		
		if (CurrentWeapon)
		{
			// If current weapon repped before PlayerState, set tag on ASC
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
			// Update owning character and ASC just in case it repped before PlayerState
			CurrentWeapon->SetOwningCharacter(this);

			if (!PrimaryReserveAmmoChangedDelegateHandle.IsValid())
			{
				PrimaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponPrimaryReserveAmmoChanged);
			}
			if (!SecondaryReserveAmmoChangedDelegateHandle.IsValid())
			{
				SecondaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->SecondaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponSecondaryReserveAmmoChanged);
			}
		}

		if (AbilitySystemComponent->GetTagCount(DeadTag) > 0)
		{
			// Set Health/Mana/Stamina/Shield to their max. This is only for *Respawn*. It will be set (replicated) by the
			// Server, but we call it here just to be a little more responsive.
			SetHealth(GetMaxHealth());
			SetMana(GetMaxMana());
			SetStamina(GetMaxStamina());
			SetShield(GetMaxShield());
		}

		// Simulated on proxies don't have their PlayerStates yet when BeginPlay is called so we call it again here
		InitializeFloatingStatusBar();
	}
}

void AGSHeroCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	SetupStartupPerspective();
}

void AGSHeroCharacter::BindASCInput()
{
	if (!bASCInputBound && IsValid(AbilitySystemComponent) && IsValid(InputComponent))
	{
		FTopLevelAssetPath AbilityEnumAssetPath = FTopLevelAssetPath(FName("/Script/GASShooter"), FName("EGSAbilityInputID"));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"),
			FString("CancelTarget"), AbilityEnumAssetPath, static_cast<int32>(EGSAbilityInputID::Confirm), static_cast<int32>(EGSAbilityInputID::Cancel)));

		bASCInputBound = true;
	}
}

void AGSHeroCharacter::SpawnDefaultInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryWeaponClasses.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (!DefaultInventoryWeaponClasses[i])
		{
			// An empty item was added to the Array in blueprint
			continue;
		}

		AGSWeapon* NewWeapon = GetWorld()->SpawnActorDeferred<AGSWeapon>(DefaultInventoryWeaponClasses[i],
			FTransform::Identity, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		NewWeapon->bSpawnWithCollision = false;
		NewWeapon->FinishSpawning(FTransform::Identity);

		bool bEquipFirstWeapon = i == 0;
		AddWeaponToInventory(NewWeapon, bEquipFirstWeapon);
	}
}

void AGSHeroCharacter::SetupStartupPerspective()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PC && PC->IsLocalController())
	{
		bIsFirstPersonPerspective = bStartInFirstPersonPerspective;
		SetPerspective(bIsFirstPersonPerspective);
	}
}

bool AGSHeroCharacter::DoesWeaponExistInInventory(AGSWeapon* InWeapon)
{
	//UE_LOG(LogTemp, Log, TEXT("%s InWeapon class %s"), *FString(__FUNCTION__), *InWeapon->GetClass()->GetName());

	for (AGSWeapon* Weapon : Inventory.Weapons)
	{
		if (Weapon && InWeapon && Weapon->GetClass() == InWeapon->GetClass())
		{
			return true;
		}
	}

	return false;
}

void AGSHeroCharacter::SetCurrentWeapon(AGSWeapon* NewWeapon, AGSWeapon* LastWeapon)
{
	if (NewWeapon == LastWeapon)
	{
		return;
	}

	// Cancel active weapon abilities
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer AbilityTagsToCancel = FGameplayTagContainer(WeaponAbilityTag);
		AbilitySystemComponent->CancelAbilities(&AbilityTagsToCancel);
	}

	UnEquipWeapon(LastWeapon);

	if (NewWeapon)
	{
		if (AbilitySystemComponent)
		{
			// Clear out potential NoWeaponTag
			AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		}

		// Weapons coming from OnRep_CurrentWeapon won't have the owner set
		CurrentWeapon = NewWeapon;
		CurrentWeapon->SetOwningCharacter(this);
		CurrentWeapon->Equip();
		CurrentWeaponTag = CurrentWeapon->WeaponTag;

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
		}

		AGSPlayerController* PC = GetController<AGSPlayerController>();
		if (PC && PC->IsLocalController())
		{
			PC->SetEquippedWeaponPrimaryIconFromSprite(CurrentWeapon->PrimaryIcon);
			PC->SetEquippedWeaponStatusText(CurrentWeapon->StatusText);
			PC->SetPrimaryClipAmmo(CurrentWeapon->GetPrimaryClipAmmo());
			PC->SetPrimaryReserveAmmo(GetPrimaryReserveAmmo());
			PC->SetHUDReticle(CurrentWeapon->GetPrimaryHUDReticleClass());
		}

		NewWeapon->OnPrimaryClipAmmoChanged.AddDynamic(this, &AGSHeroCharacter::CurrentWeaponPrimaryClipAmmoChanged);
		NewWeapon->OnSecondaryClipAmmoChanged.AddDynamic(this, &AGSHeroCharacter::CurrentWeaponSecondaryClipAmmoChanged);
		
		if (AbilitySystemComponent)
		{
			PrimaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponPrimaryReserveAmmoChanged);
			SecondaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->SecondaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponSecondaryReserveAmmoChanged);
		}

		UAnimMontage* Equip1PMontage = CurrentWeapon->GetEquip1PMontage();
		if (Equip1PMontage && GetFirstPersonMesh())
		{
			GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Equip1PMontage);
		}

		UAnimMontage* Equip3PMontage = CurrentWeapon->GetEquip3PMontage();
		if (Equip3PMontage && GetThirdPersonMesh())
		{
			GetThirdPersonMesh()->GetAnimInstance()->Montage_Play(Equip3PMontage);
		}
	}
	else
	{
		// This will clear HUD, tags etc
		UnEquipCurrentWeapon();
	}
}

void AGSHeroCharacter::UnEquipWeapon(AGSWeapon* WeaponToUnEquip)
{
	//TODO this will run into issues when calling UnEquipWeapon explicitly and the WeaponToUnEquip == CurrentWeapon

	if (WeaponToUnEquip)
	{
		WeaponToUnEquip->OnPrimaryClipAmmoChanged.RemoveDynamic(this, &AGSHeroCharacter::CurrentWeaponPrimaryClipAmmoChanged);
		WeaponToUnEquip->OnSecondaryClipAmmoChanged.RemoveDynamic(this, &AGSHeroCharacter::CurrentWeaponSecondaryClipAmmoChanged);

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(WeaponToUnEquip->PrimaryAmmoType)).Remove(PrimaryReserveAmmoChangedDelegateHandle);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(WeaponToUnEquip->SecondaryAmmoType)).Remove(SecondaryReserveAmmoChangedDelegateHandle);
		}
		
		WeaponToUnEquip->UnEquip();
	}
}

void AGSHeroCharacter::UnEquipCurrentWeapon()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		CurrentWeaponTag = NoWeaponTag;
		AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
	}

	UnEquipWeapon(CurrentWeapon);
	CurrentWeapon = nullptr;

	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalController())
	{
		PC->SetEquippedWeaponPrimaryIconFromSprite(nullptr);
		PC->SetEquippedWeaponStatusText(FText());
		PC->SetPrimaryClipAmmo(0);
		PC->SetPrimaryReserveAmmo(0);
		PC->SetHUDReticle(nullptr);
	}
}

void AGSHeroCharacter::InvokeAbility(const FInputActionValue& Value, EGSAbilityInputID Id, bool IsActive)
{
	if (IsAlive())
	{
		SendLocalInputToASC(IsActive, Id);
	}
}
//void AGSHeroCharacter::Move(const FInputActionValue& Value)
//{
//	if (IsAlive())
//	{
//		FVector2D MovementVector = Value.Get<FVector2D>();
//
//		if (Controller != nullptr)
//		{
//			const FRotator Rotation = Controller->GetControlRotation();
//			const FRotator YawRotation(0, Rotation.Yaw, 0);
//
//			FVector ForwardDirection;
//			FVector RightDirection;
//			FVector UpDirection;
//
//			FRotationMatrix(YawRotation).GetUnitAxes(ForwardDirection, RightDirection, UpDirection);
//
//			AddMovementInput(ForwardDirection, MovementVector.Y);
//			AddMovementInput(RightDirection, MovementVector.X);
//		}
//	}
//}
//
//void AGSHeroCharacter::Look(const FInputActionValue& Value)
//{
//	FVector2D LookAxisVector = Value.Get<FVector2D>();
//
//	if (Controller != nullptr)
//	{
//		AddControllerYawInput(LookAxisVector.X);
//		AddControllerPitchInput(LookAxisVector.Y);
//	}
//}

void AGSHeroCharacter::CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo)
{
	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalController())
	{
		PC->SetPrimaryClipAmmo(NewPrimaryClipAmmo);
	}
}

void AGSHeroCharacter::CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo)
{
	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalController())
	{
		PC->SetSecondaryClipAmmo(NewSecondaryClipAmmo);
	}
}

void AGSHeroCharacter::CurrentWeaponPrimaryReserveAmmoChanged(const FOnAttributeChangeData& Data)
{
	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalController())
	{
		PC->SetPrimaryReserveAmmo(Data.NewValue);
	}
}

void AGSHeroCharacter::CurrentWeaponSecondaryReserveAmmoChanged(const FOnAttributeChangeData& Data)
{
	AGSPlayerController* PC = GetController<AGSPlayerController>();
	if (PC && PC->IsLocalController())
	{
		PC->SetSecondaryReserveAmmo(Data.NewValue);
	}
}

void AGSHeroCharacter::WeaponChangingDelayReplicationTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (CallbackTag == WeaponChangingDelayReplicationTag)
	{
		if (NewCount < 1)
		{
			// We only replicate the current weapon to simulated proxies so manually sync it when the weapon changing delay replication
			// tag is removed. We keep the weapon changing tag on for ~1s after the equip montage to allow for activating changing weapon
			// again without the server trying to clobber the next locally predicted weapon.
			ClientSyncCurrentWeapon(CurrentWeapon);
		}
	}
}

void AGSHeroCharacter::OnRep_CurrentWeapon(AGSWeapon* LastWeapon)
{
	bChangedWeaponLocally = false;
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AGSHeroCharacter::OnRep_Inventory()
{
	if (GetLocalRole() == ROLE_AutonomousProxy && Inventory.Weapons.Num() > 0 && !CurrentWeapon)
	{
		// Since we don't replicate the CurrentWeapon to the owning client, this is a way to ask the Server to sync
		// the CurrentWeapon after it's been spawned via replication from the Server.
		// The weapon spawning is replicated but the variable CurrentWeapon is not on the owning client.
		ServerSyncCurrentWeapon();
	}
}

void AGSHeroCharacter::OnAbilityActivationFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags)
{
	if (FailedAbility && FailedAbility->AbilityTags.HasTagExact(FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.IsChanging"))))
	{
		if (bChangedWeaponLocally)
		{
			// Ask the Server to resync the CurrentWeapon that we predictively changed
			UE_LOG(LogTemp, Warning, TEXT("%s Weapon Changing ability activation failed. Syncing CurrentWeapon. %s. %s"), *FString(__FUNCTION__),
				*UGSBlueprintFunctionLibrary::GetPlayerEditorWindowRole(GetWorld()), *FailTags.ToString());

			ServerSyncCurrentWeapon();
		}
	}
}

void AGSHeroCharacter::ServerSyncCurrentWeapon_Implementation()
{
	ClientSyncCurrentWeapon(CurrentWeapon);
}

bool AGSHeroCharacter::ServerSyncCurrentWeapon_Validate()
{
	return true;
}

void AGSHeroCharacter::ClientSyncCurrentWeapon_Implementation(AGSWeapon* InWeapon)
{
	AGSWeapon* LastWeapon = CurrentWeapon;
	CurrentWeapon = InWeapon;
	OnRep_CurrentWeapon(LastWeapon);
}

bool AGSHeroCharacter::ClientSyncCurrentWeapon_Validate(AGSWeapon* InWeapon)
{
	return true;
}
////////////////////////////////////////////////////////////////////////////////// FP Procedural Animation Public Pure Getter (For AnimBP Property Access)

FVector AGSHeroCharacter::GetLocationLagPos() const
{
	return LocationLagPos;
}

FVector AGSHeroCharacter::GetWalkAnimPos() const
{
	return WalkAnimPos;
}

FRotator AGSHeroCharacter::GetWalkAnimRot() const
{
	return WalkAnimRot;
}

float AGSHeroCharacter::GetWalkAnimAlpha() const
{
	return WalkAnimAlpha;
}

//float AGSHeroCharacter::GetDipAlpha() const
//{
//	return DipAlpha;
//}

FVector AGSHeroCharacter::GetPitchOffsetPos() const
{
	return PitchOffsetPos;
}

FVector AGSHeroCharacter::GetCamRotOffset() const
{
	return CamRotOffset;
}

FRotator AGSHeroCharacter::GetCamRotCurrent() const
{
	return CamRotCurrent;
}

FRotator AGSHeroCharacter::GetCamRotRate() const
{
	return CamRotRate;
}

FRotator AGSHeroCharacter::GetInAirTilt() const
{
	return InAirTilt;
}

FVector AGSHeroCharacter::GetInAirOffset() const
{
	return InAirOffset;
}

FVector AGSHeroCharacter::GetCamOffsetCurrent() const
{
	return CamOffsetCurrent;
}

float AGSHeroCharacter::GetCamAnimAlpha() const
{
	return CamAnimAlpha;
}

float AGSHeroCharacter::GetADSAlpha() const
{
	return ADSAlpha;
}

float AGSHeroCharacter::GetADSAlphaInversed() const
{
	return ADSAlphaInversed;
}

float AGSHeroCharacter::GetADSAlphaLerp() const
{
	return ADSAlphaLerp;
}

FTransform AGSHeroCharacter::GetSightTransform() const
{
	return SightTransform;
}

FTransform AGSHeroCharacter::GetRelativeHandTransform() const
{
	return RelativeHandTransform;
}

float AGSHeroCharacter::GetHasWeaponAlpha() const
{
	return CurrentWeapon ? 1.f : 0.f;
}

float AGSHeroCharacter::GetCrouchAlpha() const
{
	return CrouchAlpha;
}

float AGSHeroCharacter::GetDipAlpha() const
{
	return DipAlpha;
}

////////////////////////////////////////////////////////////////////////////////// FP Procedural Animation
void AGSHeroCharacter::WalkLeftRightTLCallback(float val)
{
	WalkLeftRightAlpha = val;
}

void AGSHeroCharacter::WalkFwdBwdTLCallback(float val)
{
	WalkFwdBwdAlpha = val;
}

void AGSHeroCharacter::WalkRollTLCallback(float val)
{
	WalkRollAlpha = val;
}

//void AGSHeroCharacter::WalkTLFootstepCallback()
//{
//	if (MoveMode != ECustomMovementMode::Sliding && FootstepCue != nullptr)
//	{
//		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
//		float volumeMultiplier = FMath::Lerp(0.2f, 1.f, normalizedSpeed);
//		float pitchMultiplier = FMath::Lerp(0.8f, 1.f, normalizedSpeed);
//		UGameplayStatics::PlaySoundAtLocation(this, FootstepCue, GetActorLocation(), volumeMultiplier, pitchMultiplier);
//	}
//
//	if (MoveMode == ECustomMovementMode::Sprinting)
//	{
//		Dip(4.f, 0.35f);
//	}
//}

void AGSHeroCharacter::WalkTLUpdateEvent()
{
	if (CurrentWeapon)
	{
		// Set SightTransform
		const FTransform cameraWorldTransform = FirstPersonCamera->GetComponentTransform();
		const FTransform mesh1pWorldTransform = FirstPersonMesh->GetComponentTransform();

		const FTransform relativeTransform = UKismetMathLibrary::MakeRelativeTransform(cameraWorldTransform, mesh1pWorldTransform);

		SightTransform.SetLocation(relativeTransform.GetLocation() + relativeTransform.GetRotation().GetForwardVector() * CurrentWeapon->GetSightForwardLength());
		SightTransform.SetRotation(relativeTransform.Rotator().Quaternion());

		// Set RelativeHandTransform
		RelativeHandTransform = UKismetMathLibrary::MakeRelativeTransform(
			CurrentWeapon->GetWeaponMesh1P()->GetSocketTransform("sight"),
			FirstPersonMesh->GetSocketTransform("hand_r")
		);
	}


	// update walk anim position
	float lerpedWalkAnimPosX = FMath::Lerp(-0.4f, 0.4f, WalkLeftRightAlpha);
	float lerpedWalkAnimPosZ = FMath::Lerp(-0.35f, 0.2f, WalkFwdBwdAlpha);
	WalkAnimPos = FVector(lerpedWalkAnimPosX, 0.f, lerpedWalkAnimPosZ);

	// update walk anim rotation
	float lerpedWalkAnimRotPitch = FMath::Lerp(1.f, -1.f, WalkRollAlpha);
	WalkAnimRot = FRotator(lerpedWalkAnimRotPitch, 0.f, 0.f);

	// get alpha of walking intensity
	float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		WalkAnimAlpha = 0.f;
	}
	else
	{
		WalkAnimAlpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f); // had to clamp this because when sprinting, this would jump up beyond 1 and the footstep is too fast
	}

	float lerpedWalkAnimAlpha = FMath::Lerp(0.f, 1.65f, WalkAnimAlpha);
	WalkingTL->SetPlayRate(lerpedWalkAnimAlpha);

	// update location lag vars
	UpdateVelocityVars();

	// update look input vars
	UpdateLookInputVars(CamRotCurrent);

	// camera animation
	FVector camOffset;
	float camAnimAlpha;
	ProcCamAnim(camOffset, camAnimAlpha);
}

void AGSHeroCharacter::UpdateVelocityVars()
{
	float velocityDotForwardVec = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
	float velocityDotRightVec = FVector::DotProduct(GetVelocity(), GetActorRightVector());
	float velocityDotUpVec = FVector::DotProduct(GetVelocity(), GetActorUpVector());

	float Y = velocityDotForwardVec / (BaseWalkSpeed * -1.f);
	float X = velocityDotRightVec / BaseWalkSpeed;
	float Z = velocityDotUpVec / GetCharacterMovement()->JumpZVelocity * -1.f;

	FVector resultingVec = FVector(X, Y, Z);
	FVector scaledVec = resultingVec * 2.f;
	FVector ClampedVectorSize = scaledVec.GetClampedToSize(0.f, 4.f);

	float deltaTime = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaTime) / 6.f;
	FVector interpedVec = FMath::VInterpTo(LocationLagPos, ClampedVectorSize, deltaTime, interpSpeed);
	LocationLagPos = interpedVec;

	interpSpeed = (1.f / deltaTime) / 12.f;
	FRotator targetRInterp = FRotator((LocationLagPos.Z * -2.f), 0.f, 0.f);
	FRotator interpedRot = FMath::RInterpTo(InAirTilt, targetRInterp, deltaTime, interpSpeed);
	InAirTilt = interpedRot;

	FVector targetVInterp = FVector((LocationLagPos.Z * -0.5f), 0.f, 0.f);
	FVector interpedInAirOffsetVec = FMath::VInterpTo(InAirOffset, targetVInterp, deltaTime, interpSpeed);
	InAirOffset = interpedInAirOffsetVec;
}

void AGSHeroCharacter::UpdateLookInputVars(FRotator CamRotPrev)
{
	// Step 1: determining how much to offset the viewmodel based
	// on our current camera pitch
	FRotator deltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), GetActorRotation());
	float normalizedPitch = UKismetMathLibrary::NormalizeToRange(deltaRotator.Pitch, -90.f, 90.f);
	float lerpedY = FMath::Lerp(3.f, -3.f, normalizedPitch);
	float lerpedZ = FMath::Lerp(2.f, -2.f, normalizedPitch);
	PitchOffsetPos = FVector(0.f, lerpedY, lerpedZ);

	float normalizedFurther = UKismetMathLibrary::NormalizeToRange(normalizedPitch, 0.f, 0.5f);
	float clampedNormalizedPitch = FMath::Clamp(normalizedFurther, 0.f, 1.f);
	float lerpedClampedNormalizedPitch = FMath::Lerp(15.f, 0.f, clampedNormalizedPitch);
	FVector newRelativeLocation = FVector(lerpedClampedNormalizedPitch, FP_Root->GetRelativeLocation().Y, FP_Root->GetRelativeLocation().Z);
	FP_Root->SetRelativeLocation(newRelativeLocation);


	// Step 2: finding the rotation rate of our camera and smoothing
	// the result to use for our weapon sway
	CamRotCurrent = FirstPersonCamera->GetComponentRotation();
	FRotator deltaCamRot = UKismetMathLibrary::NormalizedDeltaRotator(CamRotCurrent, CamRotPrev);
	float deltaCamRotPitch, deltaCamRotYaw, deltaCamRotRoll;
	UKismetMathLibrary::BreakRotator(deltaCamRot, deltaCamRotRoll, deltaCamRotPitch, deltaCamRotYaw);
	float pitchInverse = deltaCamRotPitch * -1.f;
	float clampedPitchInverse = FMath::Clamp(pitchInverse, -5.f, 5.f);
	float clampedYaw = FMath::Clamp(deltaCamRotYaw, -5.f, 5.f);
	FRotator newRotator = FRotator(0.f, clampedYaw, clampedPitchInverse);
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	//float weaponWeight = bHasWeapon ? FMath::Clamp(CurrentWeapon->WeaponSwaySpeed, 6.f, 80.f) : 6.f;
	//float interpSpeed = (1.f / deltaSeconds) / weaponWeight;
	float interpSpeed = (1.f / deltaSeconds) / 36.f;
	CamRotRate = UKismetMathLibrary::RInterpTo(CamRotRate, newRotator, deltaSeconds, interpSpeed);


	// Step 3: figuring out the amount to offset our viewmodel by,
	// in order to counteract the rotation of our weapon sway
	float normalizedRoll = UKismetMathLibrary::NormalizeToRange(CamRotRate.Roll, -5.f, 5.f);
	float lerpedRoll = FMath::Lerp(-10.f, 10.f, normalizedRoll);

	float normalizedYaw = UKismetMathLibrary::NormalizeToRange(CamRotRate.Yaw, -5.f, 5.f);
	float lerpedYaw = FMath::Lerp(-6.f, 6.f, normalizedYaw);
	CamRotOffset = FVector(lerpedYaw, 0.f, lerpedRoll);
}

void AGSHeroCharacter::ProcCamAnim(FVector& CamOffsetArg, float& CamAnimAlphaArg)
{
	FTransform spine_03_Transform = FirstPersonMesh->GetSocketTransform(FName("spine_03"));
	FTransform hand_r_Transform = FirstPersonMesh->GetSocketTransform(FName("hand_r"));
	FVector inversedTransformLocation = UKismetMathLibrary::InverseTransformLocation(spine_03_Transform, hand_r_Transform.GetLocation());
	FVector differenceVec = PrevHandLoc - inversedTransformLocation;
	FVector swappedAxesVec = FVector(differenceVec.Y, differenceVec.Z, differenceVec.X);
	CamOffset = swappedAxesVec * FVector(-1.f, 1.f, -1.f);
	FVector multipliedVec = CamOffset * CamStrength;
	PrevHandLoc = inversedTransformLocation;

	UAnimInstance* meshAnimInstance = FirstPersonMesh->GetAnimInstance();
	bool isAnyMontagePlaying = meshAnimInstance->IsAnyMontagePlaying();
	auto currentActiveMontage = meshAnimInstance->GetCurrentActiveMontage();
	bool isMontageActive = meshAnimInstance->Montage_IsActive(currentActiveMontage);
	float lowerSelectFloat = isMontageActive ? 1.f : 0.f;
	float upperSelectFloat = isAnyMontagePlaying ? lowerSelectFloat : 0.f;
	float deltaSeconds = GetWorld()->DeltaTimeSeconds;
	float interpSpeed = (1.f / deltaSeconds) / 24.f;
	FVector interpedVec = UKismetMathLibrary::VInterpTo(CamOffsetCurrent, multipliedVec, deltaSeconds, interpSpeed);
	CamOffsetCurrent = interpedVec.GetClampedToSize(0.f, 10.f);

	interpSpeed = (1.f / deltaSeconds) / 60.f;
	CamAnimAlpha = UKismetMathLibrary::FInterpTo(CamAnimAlpha, upperSelectFloat, deltaSeconds, interpSpeed);

	CamOffsetArg = CamOffsetCurrent;
	CamAnimAlphaArg = CamAnimAlpha;
}

void AGSHeroCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		// change coyote time based on speed
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		float alpha = FMath::Clamp(normalizedSpeed, 0.f, 1.f);
		float lerpedValue = FMath::Lerp(0.25f, 1.f, alpha);
		float time = CoyoteTime * lerpedValue;
		GetWorldTimerManager().SetTimer(CoyoteTimerHandle, this, &ThisClass::CoyoteTimePassed, time, true);
	}
}

//void AGSHeroCharacter::CustomUnCrouch()
//{
//	GetWorldTimerManager().SetTimer(UnCrouchTimerHandle, this, &ThisClass::OnCheckCanStand, (1.f / 30.f), true);
//}
//
//void AGSHeroCharacter::OnCheckCanStand()
//{
//	FVector SphereStart = FVector(GetActorLocation().X, GetActorLocation().Y, (GetActorLocation().Z + CrouchHalfHeight));
//
//	//float lerpedHeight = FMath::Lerp(0.f, (StandHalfHeight - CrouchHalfHeight), CrouchAlpha);
//	//float scaledLerpedHeight = lerpedHeight * 1.1f;
//	//float sphereEndZ = (GetActorLocation().Z + CrouchHalfHeight) + scaledLerpedHeight;
//
//	float sphereEndZ = SphereStart.Z + (StandHalfHeight * 1.2 - CrouchHalfHeight);
//	FVector SphereEnd = FVector(GetActorLocation().X, GetActorLocation().Y, sphereEndZ);
//	float sphereRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.1f;
//	FCollisionShape Sphere{ FCollisionShape::MakeSphere(sphereRadius) };
//	FCollisionQueryParams Params = FCollisionQueryParams();
//	Params.AddIgnoredActor(this);
//	FHitResult HitResult;
//
//	bool isStuck = GetWorld()->SweepSingleByChannel(HitResult, SphereStart, SphereEnd, FQuat::Identity, ECollisionChannel::ECC_Visibility, Sphere, Params);
//	bool isFalling = GetCharacterMovement()->IsFalling();
//
//	if (!isStuck || isFalling)
//	{
//		SetIsCrouching(false);
//		StandUpFromCrouch();
//		GetWorld()->GetTimerManager().ClearTimer(UnCrouchTimerHandle);
//		UnCrouchTimerHandle.Invalidate();
//	}
//}
//
//bool AGSHeroCharacter::SetIsCrouching(bool newState)
//{
//	bIsCrouching = newState;
//	return bIsCrouching;
//}
//
//bool AGSHeroCharacter::GetIsCrouching() const
//{
//	return bIsCrouching;
//}

void AGSHeroCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	LandingDip();

	JumpsLeft = JumpsMax;

	// sequence 1
	// On landing, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();

	//// sequence 2
	//if (CrouchKeyHeld && MoveMode == ECustomMovementMode::Sprinting)
	//{
	//	ForceStartSlide();
	//}
}

void AGSHeroCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	JumpsLeft = FMath::Clamp(JumpsLeft - 1, 0, JumpsMax);
	Dip(5.f, 1.f);


	/*if (float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle); remainingTime > 0.f)
	{
		if (JumpCue != nullptr)
		{
			float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
			UGameplayStatics::PlaySoundAtLocation(this, JumpCue, GetActorLocation());
		}
	}*/

	// On jump, clear coyote timer
	GetWorld()->GetTimerManager().ClearTimer(CoyoteTimerHandle);
	CoyoteTimerHandle.Invalidate();
}

bool AGSHeroCharacter::CanJumpInternal_Implementation() const
{
	bool canJump = Super::CanJumpInternal_Implementation();
	float remainingTime = GetWorld()->GetTimerManager().GetTimerRemaining(CoyoteTimerHandle);

	//bool isSlideTLActive = SlideTL->IsActive();
	//bool selected = isSlideTLActive ? SlideTL->GetPlaybackPosition() > 0.25f : true;
	//return (canJump || remainingTime > 0.f || JumpsLeft > 0) && (!isTimerActive && selected);
	return (canJump || remainingTime > 0.f || JumpsLeft > 0);
}

void AGSHeroCharacter::CoyoteTimePassed()
{
	JumpsLeft -= 1;
}

void AGSHeroCharacter::LandingDip()
{
	float lastZVelocity = GetCharacterMovement()->GetLastUpdateVelocity().Z;
	float ZVectorLength = FVector(0.f, 0.f, lastZVelocity).Length();
	float jumpZvelocity = GetCharacterMovement()->JumpZVelocity;
	float normalizedVelocity = UKismetMathLibrary::NormalizeToRange(ZVectorLength, 0.f, jumpZvelocity);
	float clampedVelocity = FMath::Clamp(normalizedVelocity, 0.f, 1.f);
	Dip(3.f, clampedVelocity);
	/*if (LandCue != nullptr)
	{
		float normalizedSpeed = UKismetMathLibrary::NormalizeToRange(GetVelocity().Length(), 0.f, BaseWalkSpeed);
		UGameplayStatics::PlaySoundAtLocation(this, LandCue, GetActorLocation(), clampedVelocity);
	}*/
}

/// flinch for the fp viewmodel when damaged, unlike the 3p mesh hit react, this doesn't need to be multicasted since only the player will see it in fp
void AGSHeroCharacter::AddDamageIndicator(FVector SourceLocation)
{
	Super::AddDamageIndicator(SourceLocation);

	if (!GetWorldTimerManager().IsTimerActive(DamageFlinchTimer))
	{
		GetWorldTimerManager().SetTimer(DamageFlinchTimer, this, &ThisClass::FlinchFPViewmodel, 2.f, false, 0.f);
	}
}

void AGSHeroCharacter::FlinchFPViewmodel()
{
	Dip(4.f, 0.5f);
}
