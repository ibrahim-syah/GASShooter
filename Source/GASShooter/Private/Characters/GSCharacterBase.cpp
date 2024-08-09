// Copyright 2020 Dan Kestranek.


#include "Characters/GSCharacterBase.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Abilities/GSAbilitySystemGlobals.h"
#include "Characters/Abilities/GSGameplayAbility.h"
#include "Characters/GSCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "UI/GSDamageTextWidgetComponent.h"
#include "UI/GSDamageMarkerWidgetComponent.h"
#include "UI/GSKillMarkerWidgetComponent.h"
#include "UI/GSHUDDamageIndicator.h"

// Sets default values
AGSCharacterBase::AGSCharacterBase(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UGSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	bAlwaysRelevant = true;

	// Cache tags
	HitDirectionFrontTag = FGameplayTag::RequestGameplayTag(FName("Effect.HitReact.Front"));
	HitDirectionBackTag = FGameplayTag::RequestGameplayTag(FName("Effect.HitReact.Back"));
	HitDirectionRightTag = FGameplayTag::RequestGameplayTag(FName("Effect.HitReact.Right"));
	HitDirectionLeftTag = FGameplayTag::RequestGameplayTag(FName("Effect.HitReact.Left"));
	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	EffectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag("Effect.RemoveOnDeath");
	BeingTakendownTag = FGameplayTag::RequestGameplayTag("State.BeingTakendown");

	// Hardcoding to avoid having to manually set for every Blueprint child class
	DamageNumberClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/UI/DamageNumbers/WC_DamageText.WC_DamageText_C"));
	if (!DamageNumberClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find DamageNumberClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}

	DamageMarkerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/UI/HitMarker/WC_DamageMarker.WC_DamageMarker_C"));
	if (!DamageMarkerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find DamageMarkerClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}

	KillMarkerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/UI/KillMarker/WC_KillMarker.WC_KillMarker_C"));
	if (!KillMarkerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find KillMarkerClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}

	DamageIndicatorClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/UI/DamageIndicator/UI_DamageIndicator.UI_DamageIndicator_C"));
	if (!DamageIndicatorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find DamageIndicatorClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}
}

UAbilitySystemComponent* AGSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AGSCharacterBase::IsAlive() const
{
	return GetHealth() > 0.0f;
}

int32 AGSCharacterBase::GetAbilityLevel(EGSAbilityInputID AbilityID) const
{
	//TODO
	return 1;
}

void AGSCharacterBase::RemoveCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || !AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	// Remove any abilities added from a previous call. This checks to make sure the ability is in the startup 'CharacterAbilities' array.
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if ((Spec.SourceObject == this) && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	// Do in two passes so the removal happens after we have the full list
	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = false;
}

void AGSCharacterBase::Die()
{
	// Only runs on Server
	RemoveCharacterAbilities();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	OnCharacterDied.Broadcast(this);

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	//TODO replace with a locally executed GameplayCue
	/*if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}*/

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		FinishDying();
	}
}

void AGSCharacterBase::FinishDying()
{
	Destroy();
}

void AGSCharacterBase::AddDamageNumber(float Damage, FGameplayTagContainer DamageNumberTags, FVector HitLocation)
{
	DamageNumberQueue.Add(FGSDamageNumber(Damage, DamageNumberTags, HitLocation));

	if (!GetWorldTimerManager().IsTimerActive(DamageNumberTimer))
	{
		GetWorldTimerManager().SetTimer(DamageNumberTimer, this, &AGSCharacterBase::ShowDamageNumber, 0.05, true, 0.0f);
	}
}

void AGSCharacterBase::AddKillMarker(FGameplayTagContainer KillMarkerTags, FVector KillLocation)
{
	KillMarkerQueue.Add(FGSKillMarker(KillMarkerTags, KillLocation));

	if (!GetWorldTimerManager().IsTimerActive(KillMarkerTimer))
	{
		GetWorldTimerManager().SetTimer(KillMarkerTimer, this, &AGSCharacterBase::ShowKillMarker, 0.1f, true, 0.0f);
	}
}

void AGSCharacterBase::AddDamageIndicator(FVector SourceLocation)
{
	DamageIndicatorQueue.Add(FGSDamageIndicator(SourceLocation));

	if (!GetWorldTimerManager().IsTimerActive(DamageIndicatorTimer))
	{
		GetWorldTimerManager().SetTimer(DamageIndicatorTimer, this, &AGSCharacterBase::ShowDamageIndicator, 0.1f, true, 0.0f);
	}
}

bool AGSCharacterBase::IsInFirstPersonPerspective() const
{
	return bIsFirstPersonPerspective;
}

int32 AGSCharacterBase::GetCharacterLevel() const
{
	//TODO
	return 1;
}

float AGSCharacterBase::GetHealth() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetHealth();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxHealth() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxHealth();
	}
	
	return 0.0f;
}

float AGSCharacterBase::GetMana() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMana();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxMana() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxMana();
	}

	return 0.0f;
}

float AGSCharacterBase::GetStamina() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetStamina();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxStamina() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxStamina();
	}

	return 0.0f;
}

float AGSCharacterBase::GetShield() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetShield();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxShield() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxShield();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeed() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMoveSpeed();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeedBaseValue() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMoveSpeedAttribute().GetGameplayAttributeData(AttributeSetBase)->GetBaseValue();
	}

	return 0.0f;
}

// Called when the game starts or when spawned
void AGSCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AGSCharacterBase::AddCharacterAbilities()
{
	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UGSGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(StartupAbility, GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityID), static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
}

void AGSCharacterBase::InitializeAttributes()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (!DefaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's Blueprint."), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Can run on Server and Client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, GetCharacterLevel(), EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
	}
}

void AGSCharacterBase::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bStartupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
		}
	}

	AbilitySystemComponent->bStartupEffectsApplied = true;
}

void AGSCharacterBase::ShowDamageNumber()
{
	if (DamageNumberQueue.Num() > 0 && IsValid(this))
	{
		UGSDamageTextWidgetComponent* DamageText = NewObject<UGSDamageTextWidgetComponent>(this, DamageNumberClass);
		DamageText->RegisterComponent();
		DamageText->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageText->SetDamageText(DamageNumberQueue[0].DamageAmount, DamageNumberQueue[0].Tags);

		UGSDamageMarkerWidgetComponent* DamageMarker = NewObject<UGSDamageMarkerWidgetComponent>(this, DamageMarkerClass);
		DamageMarker->RegisterComponent();
		DamageMarker->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageMarker->SetRelativeLocation(GetRootComponent()->GetComponentTransform().InverseTransformPosition(DamageNumberQueue[0].HitLocation));
		DamageMarker->SetDamageMarker(DamageNumberQueue[0].Tags);

		if (DamageNumberQueue.Num() < 1)
		{
			GetWorldTimerManager().ClearTimer(DamageNumberTimer);
		}

		DamageNumberQueue.RemoveAt(0);
	}
}

void AGSCharacterBase::ShowKillMarker()
{
	if (KillMarkerQueue.Num() > 0 && IsValid(this))
	{
		UGSKillMarkerWidgetComponent* KillMarker = NewObject<UGSKillMarkerWidgetComponent>(this, KillMarkerClass);
		KillMarker->RegisterComponent();
		KillMarker->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		KillMarker->SetRelativeLocation(GetRootComponent()->GetComponentTransform().InverseTransformPosition(KillMarkerQueue[0].KilledLocation));
		KillMarker->SetKillMarker(KillMarkerQueue[0].Tags);

		if (KillMarkerQueue.Num() < 1)
		{
			GetWorldTimerManager().ClearTimer(KillMarkerTimer);
		}

		KillMarkerQueue.RemoveAt(0);
	}
}

void AGSCharacterBase::ShowDamageIndicator()
{
	if (DamageIndicatorQueue.Num() > 0 && IsValid(this))
	{
		UGSHUDDamageIndicator* DamageIndicator = CreateWidget<UGSHUDDamageIndicator>(GetLocalViewingPlayerController(), DamageIndicatorClass);
		DamageIndicator->HitLocation = DamageIndicatorQueue[0].SourceLocation;
		DamageIndicator->AddToViewport();

		if (DamageIndicatorQueue.Num() < 1)
		{
			GetWorldTimerManager().ClearTimer(DamageIndicatorTimer);
		}

		DamageIndicatorQueue.RemoveAt(0);
	}
}

void AGSCharacterBase::SetHealth(float Health)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetHealth(Health);
	}
}

void AGSCharacterBase::SetMana(float Mana)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetMana(Mana);
	}
}

void AGSCharacterBase::SetStamina(float Stamina)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetStamina(Stamina);
	}
}

void AGSCharacterBase::SetShield(float Shield)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetShield(Shield);
	}
}

EGSHitReactDirection AGSCharacterBase::GetHitReactDirection(const FVector& ImpactPoint)
{
	const FVector& ActorLocation = GetActorLocation();
	// PointPlaneDist is super cheap - 1 vector subtraction, 1 dot product.
	float DistanceToFrontBackPlane = FVector::PointPlaneDist(ImpactPoint, ActorLocation, GetActorRightVector());
	float DistanceToRightLeftPlane = FVector::PointPlaneDist(ImpactPoint, ActorLocation, GetActorForwardVector());


	if (FMath::Abs(DistanceToFrontBackPlane) <= FMath::Abs(DistanceToRightLeftPlane))
	{
		// Determine if Front or Back

		// Can see if it's left or right of Left/Right plane which would determine Front or Back
		if (DistanceToRightLeftPlane >= 0)
		{
			return EGSHitReactDirection::Front;
		}
		else
		{
			return EGSHitReactDirection::Back;
		}
	}
	else
	{
		// Determine if Right or Left

		if (DistanceToFrontBackPlane >= 0)
		{
			return EGSHitReactDirection::Right;
		}
		else
		{
			return EGSHitReactDirection::Left;
		}
	}

	return EGSHitReactDirection::Front;
}

void AGSCharacterBase::PlayHitReact_Implementation(FGameplayTag HitDirection, AActor* DamageCauser)
{
	if (IsAlive())
	{
		if (HitDirection == HitDirectionLeftTag)
		{
			ShowHitReact.Broadcast(EGSHitReactDirection::Left);
		}
		else if (HitDirection == HitDirectionFrontTag)
		{
			ShowHitReact.Broadcast(EGSHitReactDirection::Front);
		}
		else if (HitDirection == HitDirectionRightTag)
		{
			ShowHitReact.Broadcast(EGSHitReactDirection::Right);
		}
		else if (HitDirection == HitDirectionBackTag)
		{
			ShowHitReact.Broadcast(EGSHitReactDirection::Back);
		}
	}
}

bool AGSCharacterBase::PlayHitReact_Validate(FGameplayTag HitDirection, AActor* DamageCauser)
{
	return true;
}

bool AGSCharacterBase::IsAvailableForTakedown_Implementation(UPrimitiveComponent* TakedownComponent) const
{
	// Pawn is available to be takendown if HP is less than 25% and is not already being takendown.
	const float HPRatio = GetHealth() / GetMaxHealth();
	if (IsValid(AbilitySystemComponent) && (HPRatio <= 0.25f)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(BeingTakendownTag))
	{
		return true;
	}

	return IGSDamageable::IsAvailableForTakedown_Implementation(TakedownComponent);
}

float AGSCharacterBase::GetTakedownDuration_Implementation(UPrimitiveComponent* TakedownComponent) const
{
	return IGSDamageable::GetTakedownDuration_Implementation(TakedownComponent);
}

//void AGSCharacterBase::PreTakedown_Implementation(AActor* InteractingActor, UPrimitiveComponent* TakedownComponent)
//{
//	const float HPRatio = GetHealth() / GetMaxHealth();
//	if (IsValid(AbilitySystemComponent) && (HPRatio <= 0.25f) && HasAuthority())
//	{
//		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.TakenDown")));
//	}
//}

void AGSCharacterBase::PostTakedown_Implementation(AActor* InteractingActor, UPrimitiveComponent* TakedownComponent)
{
	const float HPRatio = GetHealth() / GetMaxHealth();
	if (IsValid(AbilitySystemComponent) && (HPRatio <= 0.25f) && HasAuthority())
	{
		AbilitySystemComponent->ApplyGameplayEffectToSelf(Cast<UGameplayEffect>(TakendownEffect->GetDefaultObject()), 1.0f, AbilitySystemComponent->MakeEffectContext());
	}
}

void AGSCharacterBase::GetPreTakedownSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* TakedownComponent) const
{
	const float HPRatio = GetHealth() / GetMaxHealth();
	if (IsValid(AbilitySystemComponent) && (HPRatio <= 0.25f))
	{
		bShouldSync = true;
		Type = EAbilityTaskNetSyncType::OnlyClientWait;
		return;
	}

	IGSDamageable::GetPreTakedownSyncType_Implementation(bShouldSync, Type, TakedownComponent);
}

void AGSCharacterBase::CancelTakedown_Implementation(UPrimitiveComponent* TakedownComponent)
{
	const float HPRatio = GetHealth() / GetMaxHealth();
	if (IsValid(AbilitySystemComponent) && (HPRatio <= 0.25f) && HasAuthority())
	{
		FGameplayTagContainer CancelTags(FGameplayTag::RequestGameplayTag("Ability.TakenDown"));
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}
}

FSimpleMulticastDelegate* AGSCharacterBase::GetTargetCancelTakedownDelegate(UPrimitiveComponent* TakedownComponent)
{
	return &TakedownCanceledDelegate;
}