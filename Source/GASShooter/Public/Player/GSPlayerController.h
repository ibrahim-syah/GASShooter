// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Characters/GSCharacterBase.h"
#include "GSPlayerController.generated.h"

class UPaperSprite;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 
 */
UCLASS()
class GASSHOOTER_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AGSPlayerController();

	void CreateHUD();

	class UGSHUDWidget* GetGSHUD();


	/**
	* Weapon HUD info
	*/

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetEquippedWeaponPrimaryIconFromSprite(UPaperSprite* InSprite);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetEquippedWeaponStatusText(const FText& StatusText);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetPrimaryClipAmmo(int32 ClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetPrimaryReserveAmmo(int32 ReserveAmmo);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetSecondaryClipAmmo(int32 SecondaryClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetSecondaryReserveAmmo(int32 SecondaryReserveAmmo);

	UFUNCTION(BlueprintCallable, Category = "GASShooter|UI")
	void SetHUDReticle(TSubclassOf<class UGSHUDReticle> ReticleClass);


	UFUNCTION(Client, Reliable, WithValidation)
	void ShowDamageNumber(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags, FVector HitLocation);
	void ShowDamageNumber_Implementation(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags, FVector HitLocation);
	bool ShowDamageNumber_Validate(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags, FVector HitLocation);

	UFUNCTION(Client, Reliable, WithValidation)
	void ShowKillMarker(FGameplayTagContainer KillMarkerTag, FVector KillLocation);
	void ShowKillMarker_Implementation(FGameplayTagContainer KillMarkerTag, FVector KillLocation);
	bool ShowKillMarker_Validate(FGameplayTagContainer KillMarkerTag, FVector KillLocation);

	UFUNCTION(Client, Reliable, WithValidation)
	void ShowDamageIndicator(FVector SourcheLocation);
	void ShowDamageIndicator_Implementation(FVector SourcheLocation);
	bool ShowDamageIndicator_Validate(FVector SourcheLocation);

	// Simple way to RPC to the client the countdown until they respawn from the GameMode. Will be latency amount of out sync with the Server.
	UFUNCTION(Client, Reliable, WithValidation)
	void SetRespawnCountdown(float RespawnTimeRemaining);
	void SetRespawnCountdown_Implementation(float RespawnTimeRemaining);
	bool SetRespawnCountdown_Validate(float RespawnTimeRemaining);

	UFUNCTION(Client, Reliable, WithValidation)
	void ClientSetControlRotation(FRotator NewRotation);
	void ClientSetControlRotation_Implementation(FRotator NewRotation);
	bool ClientSetControlRotation_Validate(FRotator NewRotation);

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASShooter|UI")
	TSubclassOf<class UGSHUDWidget> UIHUDWidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "GASShooter|UI")
	class UGSHUDWidget* UIHUDWidget;

	// Server only
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnRep_PlayerState() override;

	UFUNCTION(Exec)
	void Kill();

	UFUNCTION(Server, Reliable)
	void ServerKill();
	void ServerKill_Implementation();
	bool ServerKill_Validate();

protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float ADSSensitivityScale{ 0.3f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookMouseAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookStickAction = nullptr;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	//UInputAction* CrouchAction = nullptr;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	int64 MaxRotCache = 3;
	float RotArrayX[3];
	float RotArrayY[3];
	int64 RotCacheIndex = 0;

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_LookStick(const FInputActionValue& InputActionValue);
	//void Input_Crouch(const FInputActionValue& InputActionValue);
	//void Input_CrouchRelease(const FInputActionValue& InputActionValue);
};
