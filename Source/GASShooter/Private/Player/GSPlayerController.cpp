// Copyright 2020 Dan Kestranek.


#include "Player/GSPlayerController.h"
#include "Characters/Abilities/AttributeSets/GSAmmoAttributeSet.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Heroes/GSHeroCharacter.h"
#include "Player/GSPlayerState.h"
#include "UI/GSHUDWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "Weapons/GSWeapon.h"

AGSPlayerController::AGSPlayerController() :
	RotArrayX { 0.f, 0.f, 0.f },
	RotArrayY { 0.f, 0.f, 0.f }
{
}

void AGSPlayerController::CreateHUD()
{
	// Only create once
	if (UIHUDWidget)
	{
		return;
	}

	if (!UIHUDWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing UIHUDWidgetClass. Please fill in on the Blueprint of the PlayerController."), *FString(__FUNCTION__));
		return;
	}

	// Only create a HUD for local player
	if (!IsLocalPlayerController())
	{
		return;
	}

	// Need a valid PlayerState to get attributes from
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (!PS)
	{
		return;
	}

	UIHUDWidget = CreateWidget<UGSHUDWidget>(this, UIHUDWidgetClass);
	UIHUDWidget->AddToViewport();

	// Set attributes
	UIHUDWidget->SetCurrentHealth(PS->GetHealth());
	UIHUDWidget->SetMaxHealth(PS->GetMaxHealth());
	UIHUDWidget->SetHealthPercentage(PS->GetHealth() / PS->GetMaxHealth());
	UIHUDWidget->SetCurrentMana(PS->GetMana());
	UIHUDWidget->SetMaxMana(PS->GetMaxMana());
	UIHUDWidget->SetManaPercentage(PS->GetMana() / PS->GetMaxMana());
	UIHUDWidget->SetHealthRegenRate(PS->GetHealthRegenRate());
	UIHUDWidget->SetManaRegenRate(PS->GetManaRegenRate());
	UIHUDWidget->SetCurrentStamina(PS->GetStamina());
	UIHUDWidget->SetMaxStamina(PS->GetMaxStamina());
	UIHUDWidget->SetStaminaPercentage(PS->GetStamina() / PS->GetMaxStamina());
	UIHUDWidget->SetStaminaRegenRate(PS->GetStaminaRegenRate());
	UIHUDWidget->SetCurrentShield(PS->GetShield());
	UIHUDWidget->SetMaxShield(PS->GetMaxShield());
	UIHUDWidget->SetShieldRegenRate(PS->GetShieldRegenRate());
	UIHUDWidget->SetShieldPercentage(PS->GetShield() / PS->GetMaxShield());
	UIHUDWidget->SetExperience(PS->GetXP());
	UIHUDWidget->SetGold(PS->GetGold());
	UIHUDWidget->SetHeroLevel(PS->GetCharacterLevel());

	AGSHeroCharacter* Hero = GetPawn<AGSHeroCharacter>();
	if (Hero)
	{
		AGSWeapon* CurrentWeapon = Hero->GetCurrentWeapon();
		if (CurrentWeapon)
		{
			UIHUDWidget->SetEquippedWeaponSprite(CurrentWeapon->PrimaryIcon);
			UIHUDWidget->SetEquippedWeaponStatusText(CurrentWeapon->GetDefaultStatusText());
			UIHUDWidget->SetPrimaryClipAmmo(Hero->GetPrimaryClipAmmo());
			UIHUDWidget->SetReticle(CurrentWeapon->GetPrimaryHUDReticleClass());

			// PlayerState's Pawn isn't set up yet so we can't just call PS->GetPrimaryReserveAmmo()
			if (PS->GetAmmoAttributeSet())
			{
				FGameplayAttribute Attribute = PS->GetAmmoAttributeSet()->GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType);
				if (Attribute.IsValid())
				{
					UIHUDWidget->SetPrimaryReserveAmmo(PS->GetAbilitySystemComponent()->GetNumericAttribute(Attribute));
				}
			}
		}
	}
}

UGSHUDWidget* AGSPlayerController::GetGSHUD()
{
	return UIHUDWidget;
}

void AGSPlayerController::SetEquippedWeaponPrimaryIconFromSprite(UPaperSprite* InSprite)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetEquippedWeaponSprite(InSprite);
	}
}

void AGSPlayerController::SetEquippedWeaponStatusText(const FText& StatusText)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetEquippedWeaponStatusText(StatusText);
	}
}

void AGSPlayerController::SetPrimaryClipAmmo(int32 ClipAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetPrimaryClipAmmo(ClipAmmo);
	}
}

void AGSPlayerController::SetPrimaryReserveAmmo(int32 ReserveAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetPrimaryReserveAmmo(ReserveAmmo);
	}
}

void AGSPlayerController::SetSecondaryClipAmmo(int32 SecondaryClipAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetSecondaryClipAmmo(SecondaryClipAmmo);
	}
}

void AGSPlayerController::SetSecondaryReserveAmmo(int32 SecondaryReserveAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetSecondaryReserveAmmo(SecondaryReserveAmmo);
	}
}

void AGSPlayerController::SetHUDReticle(TSubclassOf<UGSHUDReticle> ReticleClass)
{
	// !GetWorld()->bIsTearingDown Stops an error when quitting PIE while targeting when the EndAbility resets the HUD reticle
	if (UIHUDWidget && GetWorld() && !GetWorld()->bIsTearingDown)
	{
		UIHUDWidget->SetReticle(ReticleClass);
	}
}

void AGSPlayerController::ShowDamageNumber_Implementation(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags, FVector HitLocation)
{
	if (IsValid(TargetCharacter))
	{
		TargetCharacter->AddDamageNumber(DamageAmount, DamageNumberTags, HitLocation);
	}
}

bool AGSPlayerController::ShowDamageNumber_Validate(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags, FVector HitLocation)
{
	return true;
}

void AGSPlayerController::ShowKillMarker_Implementation(FGameplayTagContainer KillMarkerTag, FVector KillLocation)
{
	GetPawn<AGSCharacterBase>()->AddKillMarker(KillMarkerTag, KillLocation);
}

bool AGSPlayerController::ShowKillMarker_Validate(FGameplayTagContainer KillMarkerTag, FVector KillLocation)
{
	return true;
}

void AGSPlayerController::ShowDamageIndicator_Implementation(FVector SourceLocation)
{
	if (AGSCharacterBase* HeroCharacter = GetPawn<AGSCharacterBase>())
	{
		HeroCharacter->AddDamageIndicator(SourceLocation);
	}
}

bool AGSPlayerController::ShowDamageIndicator_Validate(FVector SourceLocation)
{
	return true;
}

void AGSPlayerController::SetRespawnCountdown_Implementation(float RespawnTimeRemaining)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetRespawnCountdown(RespawnTimeRemaining);
	}
}

bool AGSPlayerController::SetRespawnCountdown_Validate(float RespawnTimeRemaining)
{
	return true;
}

void AGSPlayerController::ClientSetControlRotation_Implementation(FRotator NewRotation)
{
	SetControlRotation(NewRotation);
}

bool AGSPlayerController::ClientSetControlRotation_Validate(FRotator NewRotation)
{
	return true;
}

void AGSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		// Init ASC with PS (Owner) and our new Pawn (AvatarActor)
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}
}

void AGSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// For edge cases where the PlayerState is repped before the Hero is possessed.
	CreateHUD();
}

void AGSPlayerController::Kill()
{
	ServerKill();
}

void AGSPlayerController::ServerKill_Implementation()
{
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		PS->GetAttributeSetBase()->SetHealth(0.0f);
	}
}

bool AGSPlayerController::ServerKill_Validate()
{
	return true;
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		UE_LOG(LogTemp, Warning, TEXT("BeginPlay"));
	}
}

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
		//EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopMove);

		// Looking
		EnhancedInputComponent->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse);
		EnhancedInputComponent->BindAction(LookStickAction, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick);

		// Crouch
		//EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_Crouch);
		//EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::Input_CrouchRelease);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AGSPlayerController::Input_Move(const FInputActionValue& InputActionValue)
{
	AGSHeroCharacter* HeroCharacter = GetPawn<AGSHeroCharacter>();
	AController* Controller = HeroCharacter ? HeroCharacter->GetController() : nullptr;

	if (Controller && HeroCharacter->IsAlive())
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			HeroCharacter->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			HeroCharacter->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void AGSPlayerController::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	AGSHeroCharacter* HeroCharacter = GetPawn<AGSHeroCharacter>();

	if (!HeroCharacter)
	{
		return;
	}
	float LookScaleModifier = 1.f;
	LookScaleModifier *= FMath::Lerp(1.f, ADSSensitivityScale, HeroCharacter->GetADSAlpha());
	const FVector2D Value = InputActionValue.Get<FVector2D>() * LookScaleModifier;

	// Supposedly, this "last three frames average" would help smooth out the look control.
	// but I can't feel the difference, but idk i'll keep using it until someone notices.

	/*if (Value.X != 0.0f)
	{
		HeroCharacter->AddControllerYawInput(Value.X);
	}*/

	RotArrayX[RotCacheIndex] = Value.X;
	float result = 0.f;
	for (int i = 0; i < MaxRotCache; i++)
	{
		result += RotArrayX[i];
	}
	HeroCharacter->AddControllerYawInput(result / MaxRotCache);

	/*if (Value.Y != 0.0f)
	{
		HeroCharacter->AddControllerPitchInput(Value.Y);
	}*/

	RotArrayY[RotCacheIndex] = Value.Y;
	result = 0.f;
	for (int i = 0; i < MaxRotCache; i++)
	{
		result += RotArrayY[i];
	}
	HeroCharacter->AddControllerPitchInput(result / MaxRotCache);

	RotCacheIndex++;
	RotCacheIndex %= MaxRotCache;

	AGSWeapon* currentWeapon = HeroCharacter->GetCurrentWeapon();
	if (currentWeapon && currentWeapon->bIsRecoilPitchRecoveryActive)
	{
		FRotator currentRotation = GetControlRotation();
		FRotator checkpointRotation = currentWeapon->RecoilCheckpoint;

		FRotator deltaRot = (currentRotation - checkpointRotation).GetNormalized();

		if (Value.Y < 0.f)
		{
			currentWeapon->bIsRecoilPitchRecoveryActive = false;
			currentWeapon->bIsRecoilNeutral = true;
			return;
		}

		if (deltaRot.Pitch < 0.f)
		{
			currentWeapon->bUpdateRecoilPitchCheckpointInNextShot = true;
		}

		if (Value.X != 0.f)
		{
			if (currentWeapon->bIsRecoilYawRecoveryActive)
			{
				currentWeapon->bIsRecoilYawRecoveryActive = false;
			}

			currentWeapon->bUpdateRecoilYawCheckpointInNextShot = true;
		}

	}
}

void AGSPlayerController::Input_LookStick(const FInputActionValue& InputActionValue)
{
	AGSHeroCharacter* HeroCharacter = GetPawn<AGSHeroCharacter>();

	if (!HeroCharacter)
	{
		return;
	}
	float LookScaleModifier = 1.f;
	LookScaleModifier *= FMath::Lerp(1.f, ADSSensitivityScale, HeroCharacter->GetADSAlpha());
	const FVector2D Value = InputActionValue.Get<FVector2D>() * LookScaleModifier;

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		HeroCharacter->AddControllerYawInput(Value.X * 1.f * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		HeroCharacter->AddControllerPitchInput(Value.Y * 1.f * World->GetDeltaSeconds());
	}
}

//void AGSPlayerController::Input_Crouch(const FInputActionValue& InputActionValue)
//{
//	if (AGSHeroCharacter* HeroCharacter = GetPawn<AGSHeroCharacter>())
//	{
//		HeroCharacter->Crouch();
//	}
//}
//
//void AGSPlayerController::Input_CrouchRelease(const FInputActionValue& InputActionValue)
//{
//	if (AGSHeroCharacter* HeroCharacter = GetPawn<AGSHeroCharacter>())
//	{
//		HeroCharacter->UnCrouch();
//	}
//}
