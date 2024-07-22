// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/AttributeSets/GSAmmoAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UGSAmmoAttributeSet::UGSAmmoAttributeSet()
{
	NormalAmmoTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Normal"));
	HeavyAmmoTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Heavy"));
	SpecialAmmoTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Special"));
}

void UGSAmmoAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UGSAmmoAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetNormalReserveAmmoAttribute())
	{
		float Ammo = GetNormalReserveAmmo();
		SetNormalReserveAmmo(FMath::Clamp<float>(Ammo, 0, GetMaxNormalReserveAmmo()));
	}
	else if (Data.EvaluatedData.Attribute == GetHeavyReserveAmmoAttribute())
	{
		float Ammo = GetHeavyReserveAmmo();
		SetHeavyReserveAmmo(FMath::Clamp<float>(Ammo, 0, GetMaxHeavyReserveAmmo()));
	}
	else if (Data.EvaluatedData.Attribute == GetSpecialReserveAmmoAttribute())
	{
		float Ammo = GetSpecialReserveAmmo();
		SetSpecialReserveAmmo(FMath::Clamp<float>(Ammo, 0, GetMaxSpecialReserveAmmo()));
	}
}

void UGSAmmoAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, NormalReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, MaxNormalReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, HeavyReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, MaxHeavyReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, SpecialReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAmmoAttributeSet, MaxSpecialReserveAmmo, COND_None, REPNOTIFY_Always);
}

FGameplayAttribute UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(FGameplayTag& PrimaryAmmoTag)
{
	if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Normal")))
	{
		return GetNormalReserveAmmoAttribute();
	}
	else if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Heavy")))
	{
		return GetHeavyReserveAmmoAttribute();
	}
	else if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Special")))
	{
		return GetSpecialReserveAmmoAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UGSAmmoAttributeSet::GetMaxReserveAmmoAttributeFromTag(FGameplayTag& PrimaryAmmoTag)
{
	if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Normal")))
	{
		return GetMaxNormalReserveAmmoAttribute();
	}
	else if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Heavy")))
	{
		return GetMaxHeavyReserveAmmoAttribute();
	}
	else if (PrimaryAmmoTag == FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.Special")))
	{
		return GetMaxSpecialReserveAmmoAttribute();
	}

	return FGameplayAttribute();
}

void UGSAmmoAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void UGSAmmoAttributeSet::OnRep_NormalReserveAmmo(const FGameplayAttributeData& OldNormalReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, NormalReserveAmmo, OldNormalReserveAmmo);
}

void UGSAmmoAttributeSet::OnRep_MaxNormalReserveAmmo(const FGameplayAttributeData& OldMaxNormalReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, MaxNormalReserveAmmo, OldMaxNormalReserveAmmo);
}

void UGSAmmoAttributeSet::OnRep_HeavyReserveAmmo(const FGameplayAttributeData& OldHeavyReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, HeavyReserveAmmo, OldHeavyReserveAmmo);
}

void UGSAmmoAttributeSet::OnRep_MaxHeavyReserveAmmo(const FGameplayAttributeData& OldMaxHeavyReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, MaxHeavyReserveAmmo, OldMaxHeavyReserveAmmo);
}

void UGSAmmoAttributeSet::OnRep_SpecialReserveAmmo(const FGameplayAttributeData& OldSpecialReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, SpecialReserveAmmo, OldSpecialReserveAmmo);
}

void UGSAmmoAttributeSet::OnRep_MaxSpecialReserveAmmo(const FGameplayAttributeData& OldMaxSpecialReserveAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAmmoAttributeSet, MaxSpecialReserveAmmo, OldMaxSpecialReserveAmmo);
}
