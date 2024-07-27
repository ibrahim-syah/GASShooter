// Copyright 2020 Dan Kestranek.


#include "Characters/GSCharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/Abilities/GSAbilitySystemGlobals.h"
#include "Characters/GSCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameplayTagContainer.h"

UGSCharacterMovementComponent::UGSCharacterMovementComponent()
{
	SprintSpeedMultiplier = 1.4f;
	ADSSpeedMultiplier = 0.55f;
	CrouchSpeedMultiplier = 0.65;
	KnockedDownSpeedMultiplier = 0.4f;

	KnockedDownTag = FGameplayTag::RequestGameplayTag("State.KnockedDown");
	InteractingTag = FGameplayTag::RequestGameplayTag("State.Interacting");
	InteractingRemovalTag = FGameplayTag::RequestGameplayTag("State.InteractingRemoval");
}

float UGSCharacterMovementComponent::GetMaxSpeed() const
{
	AGSCharacterBase* Owner = Cast<AGSCharacterBase>(GetOwner());
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() No Owner"), *FString(__FUNCTION__));
		return Super::GetMaxSpeed();
	}

	if (!Owner->IsAlive())
	{
		return 0.0f;
	}

	// Don't move while interacting or being interacted on (revived)
	if (Owner->GetAbilitySystemComponent() && Owner->GetAbilitySystemComponent()->GetTagCount(InteractingTag)
		> Owner->GetAbilitySystemComponent()->GetTagCount(InteractingRemovalTag))
	{
		return 0.0f;
	}

	if (Owner->GetAbilitySystemComponent() && Owner->GetAbilitySystemComponent()->HasMatchingGameplayTag(KnockedDownTag))
	{
		return Owner->GetMoveSpeed() * KnockedDownSpeedMultiplier;
	}

	if (RequestToStartSprinting)
	{
		return Owner->GetMoveSpeed() * SprintSpeedMultiplier;
	}

	// ADS and crouch can happen simultanously, so we only return the slowest of the two (ADS) when both are requested
	if (RequestToStartADS)
	{
		return Owner->GetMoveSpeed() * ADSSpeedMultiplier;
	}

	if (RequestToStartCrouching)
	{
		return Owner->GetMoveSpeed() * CrouchSpeedMultiplier;
	}

	return Owner->GetMoveSpeed();
}

void UGSCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//The Flags parameter contains the compressed input flags that are stored in the saved move.
	//UpdateFromCompressed flags simply copies the flags from the saved move into the movement component.
	//It basically just resets the movement component to the state when the move was made so it can simulate from there.
	RequestToStartSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;

	RequestToStartADS = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* UGSCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);

	if (!ClientPredictionData)
	{
		UGSCharacterMovementComponent* MutableThis = const_cast<UGSCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FGSNetworkPredictionData_Client(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void UGSCharacterMovementComponent::StartSprinting()
{
	RequestToStartSprinting = true;
}

void UGSCharacterMovementComponent::StopSprinting()
{
	RequestToStartSprinting = false;
}

void UGSCharacterMovementComponent::StartAimDownSights()
{
	RequestToStartADS = true;
}

void UGSCharacterMovementComponent::StopAimDownSights()
{
	RequestToStartADS = false;
}

void UGSCharacterMovementComponent::StartCrouching()
{
	RequestToStartCrouching = true;
}

void UGSCharacterMovementComponent::StopCrouching()
{
	RequestToStartCrouching = false;
}

const FLyraCharacterGroundInfo& UGSCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LyraCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

void UGSCharacterMovementComponent::FGSSavedMove::Clear()
{
	Super::Clear();

	SavedRequestToStartSprinting = false;
	SavedRequestToStartADS = false;
}

uint8 UGSCharacterMovementComponent::FGSSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (SavedRequestToStartSprinting)
	{
		Result |= FLAG_Custom_0;
	}

	if (SavedRequestToStartADS)
	{
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool UGSCharacterMovementComponent::FGSSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	//Set which moves can be combined together. This will depend on the bit flags that are used.
	if (SavedRequestToStartSprinting != ((FGSSavedMove*)NewMove.Get())->SavedRequestToStartSprinting)
	{
		return false;
	}

	if (SavedRequestToStartADS != ((FGSSavedMove*)NewMove.Get())->SavedRequestToStartADS)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void UGSCharacterMovementComponent::FGSSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UGSCharacterMovementComponent* CharacterMovement = Cast<UGSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
		SavedRequestToStartSprinting = CharacterMovement->RequestToStartSprinting;
		SavedRequestToStartADS = CharacterMovement->RequestToStartADS;
	}
}

void UGSCharacterMovementComponent::FGSSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UGSCharacterMovementComponent* CharacterMovement = Cast<UGSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
	}
}

UGSCharacterMovementComponent::FGSNetworkPredictionData_Client::FGSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UGSCharacterMovementComponent::FGSNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FGSSavedMove());
}

