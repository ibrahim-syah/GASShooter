// Copyright 2020 Dan Kestranek.


#include "Weapons/GSWeapon.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Abilities/GSAbilitySystemGlobals.h"
#include "Characters/Abilities/GSGameplayAbility.h"
#include "Characters/Abilities/GSGATA_LineTrace.h"
#include "Characters/Abilities/GSGATA_LineTraceWithBloom.h"
#include "Characters/Abilities/GSGATA_SphereTrace.h"
#include "Characters/Heroes/GSHeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GSBlueprintFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/GSPlayerController.h"
#include <random>

// Sets default values
AGSWeapon::AGSWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	NetUpdateFrequency = 100.0f; // Set this to a value that's appropriate for your game
	bSpawnWithCollision = true;
	PrimaryClipAmmo = 0;
	MaxPrimaryClipAmmo = 0;
	SecondaryClipAmmo = 0;
	MaxSecondaryClipAmmo = 0;
	bInfiniteAmmo = false;
	PrimaryAmmoType = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));
	SecondaryAmmoType = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionComponent"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Manually enable when in pickup mode
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(FName("WeaponMesh1P"));
	WeaponMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh1P->CastShadow = false;
	WeaponMesh1P->SetVisibility(false, true);
	WeaponMesh1P->SetupAttachment(CollisionComp);
	WeaponMesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	WeaponMesh1P->BoundsScale = 2.f;

	WeaponMesh3PickupRelativeLocation = FVector(0.0f, -25.0f, 0.0f);

	WeaponMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(FName("WeaponMesh3P"));
	WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh3P->SetupAttachment(CollisionComp);
	WeaponMesh3P->SetRelativeLocation(WeaponMesh3PickupRelativeLocation);
	WeaponMesh3P->CastShadow = true;
	WeaponMesh3P->SetVisibility(true, true);
	WeaponMesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	WeaponMesh3P->BoundsScale = 2.f;

	WeaponPrimaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Primary.Instant");
	WeaponSecondaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Secondary.Instant");
	WeaponAlternateInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Alternate.Instant");
	WeaponIsFiringTag = FGameplayTag::RequestGameplayTag("Weapon.IsFiring");

	FireMode = FGameplayTag::RequestGameplayTag("Weapon.FireMode.None");
	FullAutoFireMode = FGameplayTag::RequestGameplayTag("Weapon.FireMode.FullAuto");
	StatusText = DefaultStatusText;

	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.KnockedDown"));
}

void AGSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(OwningCharacter) || !IsValid(OwningCharacter->GetController()))
	{
		return;
	}

	if (bIsRecoilActive)
	{
		OwningCharacter->AddControllerPitchInput(RecoilPitchVelocity * -1.f * DeltaTime);
		OwningCharacter->AddControllerYawInput(RecoilYawVelocity * DeltaTime);

		RecoilPitchVelocity -= RecoilPitchDamping * DeltaTime;
		RecoilYawVelocity -= RecoilYawDamping * DeltaTime;

		if (RecoilPitchVelocity <= 0.0f)
		{
			bIsRecoilActive = false;
			StartRecoilRecovery();
		}
	}
	else if (bIsRecoilPitchRecoveryActive)
	{
		FRotator currentControlRotation = OwningCharacter->GetControlRotation();

		FRotator deltaRot = currentControlRotation - RecoilCheckpoint;
		deltaRot.Normalize();

		if (deltaRot.Pitch > 1.f)
		{
			float interpSpeed = (1.f / DeltaTime) / 4.f;
			FRotator interpRot = FMath::RInterpConstantTo(currentControlRotation, RecoilCheckpoint, DeltaTime, interpSpeed);
			interpSpeed = (1.f / DeltaTime) / 10.f; // TODO: figure out how to make dynamic yaw recovery speed that depends on the pitch distance so that the pitch and yaw recovery ands together smoothly
			interpRot.Yaw = FMath::RInterpTo(currentControlRotation, RecoilCheckpoint, DeltaTime, interpSpeed).Yaw;
			if (!bIsRecoilYawRecoveryActive)
			{
				interpRot.Yaw = currentControlRotation.Yaw;
			}

			OwningCharacter->GetController()->SetControlRotation(interpRot);
		}
		else if (deltaRot.Pitch > 0.1f)
		{
			float interpSpeed = (1.f / DeltaTime) / 6.f;
			FRotator interpRot = FMath::RInterpTo(currentControlRotation, RecoilCheckpoint, DeltaTime, interpSpeed);
			if (!bIsRecoilYawRecoveryActive)
			{
				interpRot.Yaw = currentControlRotation.Yaw;
			}
			OwningCharacter->GetController()->SetControlRotation(interpRot);
		}
		else
		{
			bIsRecoilPitchRecoveryActive = false;
			bIsRecoilYawRecoveryActive = false;
			bIsRecoilNeutral = true;
		}
	}

	if (CurrentTargetingSpread > 0.f)
	{
		float interpSpeed = (1.f / DeltaTime) / SpreadRecoveryInterpSpeed;
		CurrentTargetingSpread = FMath::FInterpConstantTo(CurrentTargetingSpread, 0.f, DeltaTime, interpSpeed);
	}
}

UAbilitySystemComponent* AGSWeapon::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USkeletalMeshComponent* AGSWeapon::GetWeaponMesh1P() const
{
	return WeaponMesh1P;
}

USkeletalMeshComponent* AGSWeapon::GetWeaponMesh3P() const
{
	return WeaponMesh3P;
}

void AGSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AGSWeapon, OwningCharacter, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSWeapon, PrimaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSWeapon, MaxPrimaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSWeapon, SecondaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSWeapon, MaxSecondaryClipAmmo, COND_OwnerOnly);
}

void AGSWeapon::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE(AGSWeapon, PrimaryClipAmmo, (IsValid(AbilitySystemComponent) && !AbilitySystemComponent->HasMatchingGameplayTag(WeaponIsFiringTag)));
	DOREPLIFETIME_ACTIVE_OVERRIDE(AGSWeapon, SecondaryClipAmmo, (IsValid(AbilitySystemComponent) && !AbilitySystemComponent->HasMatchingGameplayTag(WeaponIsFiringTag)));
}

void AGSWeapon::SetOwningCharacter(AGSHeroCharacter* InOwningCharacter)
{
	OwningCharacter = InOwningCharacter;
	if (OwningCharacter)
	{
		// Called when added to inventory
		AbilitySystemComponent = Cast<UGSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());
		SetOwner(InOwningCharacter);
		AttachToComponent(OwningCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (OwningCharacter->GetCurrentWeapon() != this)
		{
			WeaponMesh3P->CastShadow = false;
			WeaponMesh3P->SetVisibility(true, true);
			WeaponMesh3P->SetVisibility(false, true);
		}
	}
	else
	{
		AbilitySystemComponent = nullptr;
		SetOwner(nullptr);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AGSWeapon::NotifyActorBeginOverlap(AActor* Other)
{
	Super::NotifyActorBeginOverlap(Other);

	if (IsValidChecked(this) && !OwningCharacter)
	{
		PickUpOnTouch(Cast<AGSHeroCharacter>(Other));
	}
}

void AGSWeapon::Equip()
{
	if (!OwningCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("%s %s OwningCharacter is nullptr"), *FString(__FUNCTION__), *GetName());
		return;
	}

	FName AttachPoint = OwningCharacter->GetWeaponAttachPoint();

	if (WeaponMesh1P)
	{
		WeaponMesh1P->AttachToComponent(OwningCharacter->GetFirstPersonMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachPoint);
		WeaponMesh1P->SetRelativeLocation(WeaponMesh1PEquippedRelativeLocation);
		WeaponMesh1P->SetRelativeRotation(WeaponMesh1PEquippedRelativeRotation);

		OwningCharacter->SetOffsetRootLocationOffset(OffsetRootLocationOffset);

		if (OwningCharacter->IsInFirstPersonPerspective())
		{
			WeaponMesh1P->SetVisibility(true, true);
		}
		else
		{
			WeaponMesh1P->SetVisibility(false, true);
		}

		if (WeaponAnimLinkLayer1P)
		{
			OwningCharacter->GetFirstPersonMesh()->LinkAnimClassLayers(WeaponAnimLinkLayer1P);
		}
	}

	if (WeaponMesh3P)
	{
		WeaponMesh3P->AttachToComponent(OwningCharacter->GetThirdPersonMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, AttachPoint);
		WeaponMesh3P->SetRelativeLocation(WeaponMesh3PEquippedRelativeLocation);
		WeaponMesh3P->SetRelativeRotation(WeaponMesh3PEquippedRelativeRotation);
		WeaponMesh3P->CastShadow = true;
		WeaponMesh3P->bCastHiddenShadow = true;

		if (OwningCharacter->IsInFirstPersonPerspective())
		{
			WeaponMesh3P->SetVisibility(true, true); // Without this, the weapon's 3p shadow doesn't show
			WeaponMesh3P->SetVisibility(false, true);
		}
		else
		{
			WeaponMesh3P->SetVisibility(true, true);
		}

		if (WeaponAnimLinkLayer3P)
		{
			OwningCharacter->GetThirdPersonMesh()->LinkAnimClassLayers(WeaponAnimLinkLayer3P);
		}
	}
}

void AGSWeapon::UnEquip()
{
	if (OwningCharacter == nullptr)
	{
		return;
	}

	// Necessary to detach so that when toggling perspective all meshes attached won't become visible.

	WeaponMesh1P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	WeaponMesh1P->SetVisibility(false, true);
	OwningCharacter->SetOffsetRootLocationOffset(FVector(0.f));

	WeaponMesh3P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	WeaponMesh3P->CastShadow = false;
	WeaponMesh3P->bCastHiddenShadow = false;
	WeaponMesh3P->SetVisibility(true, true); // Without this, the unequipped weapon's 3p shadow hangs around
	WeaponMesh3P->SetVisibility(false, true);
}

void AGSWeapon::AddAbilities()
{
	if (!IsValid(OwningCharacter) || !OwningCharacter->GetAbilitySystemComponent())
	{
		return;
	}

	UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());

	if (!ASC)
	{
		//UE_LOG(LogTemp, Error, TEXT("%s %s Role: %s ASC is null"), *FString(__FUNCTION__), *GetName(), GET_ACTOR_ROLE_FSTRING(OwningCharacter));
		UE_LOG(LogTemp, Error, TEXT("%s %s ASC Role is null"), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (TSubclassOf<UGSGameplayAbility>& Ability : Abilities)
	{
		AbilitySpecHandles.Add(ASC->GiveAbility(
			FGameplayAbilitySpec(Ability, GetAbilityLevel(Ability.GetDefaultObject()->AbilityID), static_cast<int32>(Ability.GetDefaultObject()->AbilityInputID), this)));
	}
}

void AGSWeapon::RemoveAbilities()
{
	if (!IsValid(OwningCharacter) || !OwningCharacter->GetAbilitySystemComponent())
	{
		return;
	}

	UGSAbilitySystemComponent* ASC = Cast<UGSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());

	if (!ASC)
	{
		return;
	}

	// Remove abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (FGameplayAbilitySpecHandle& SpecHandle : AbilitySpecHandles)
	{
		ASC->ClearAbility(SpecHandle);
	}
}

int32 AGSWeapon::GetAbilityLevel(EGSAbilityInputID AbilityID)
{
	// All abilities for now are level 1
	return 1;
}

void AGSWeapon::ResetWeapon()
{
	FireMode = DefaultFireMode;
	StatusText = DefaultStatusText;
}

void AGSWeapon::OnDropped_Implementation(FVector NewLocation)
{
	SetOwningCharacter(nullptr);
	ResetWeapon();

	SetActorLocation(NewLocation);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (WeaponMesh1P)
	{
		WeaponMesh1P->AttachToComponent(CollisionComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
		WeaponMesh1P->SetVisibility(false, true);
	}

	if (WeaponMesh3P)
	{
		WeaponMesh3P->AttachToComponent(CollisionComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
		WeaponMesh3P->SetRelativeLocation(WeaponMesh3PickupRelativeLocation);
		WeaponMesh3P->CastShadow = true;
		WeaponMesh3P->SetVisibility(true, true);
	}
}

bool AGSWeapon::OnDropped_Validate(FVector NewLocation)
{
	return true;
}

int32 AGSWeapon::GetPrimaryClipAmmo() const
{
	return PrimaryClipAmmo;
}

int32 AGSWeapon::GetMaxPrimaryClipAmmo() const
{
	return MaxPrimaryClipAmmo;
}

int32 AGSWeapon::GetSecondaryClipAmmo() const
{
	return SecondaryClipAmmo;
}

int32 AGSWeapon::GetMaxSecondaryClipAmmo() const
{
	return MaxSecondaryClipAmmo;
}

void AGSWeapon::SetPrimaryClipAmmo(int32 NewPrimaryClipAmmo)
{
	int32 OldPrimaryClipAmmo = PrimaryClipAmmo;
	PrimaryClipAmmo = NewPrimaryClipAmmo;
	OnPrimaryClipAmmoChanged.Broadcast(OldPrimaryClipAmmo, PrimaryClipAmmo);
}

void AGSWeapon::SetMaxPrimaryClipAmmo(int32 NewMaxPrimaryClipAmmo)
{
	int32 OldMaxPrimaryClipAmmo = MaxPrimaryClipAmmo;
	MaxPrimaryClipAmmo = NewMaxPrimaryClipAmmo;
	OnMaxPrimaryClipAmmoChanged.Broadcast(OldMaxPrimaryClipAmmo, MaxPrimaryClipAmmo);
}

void AGSWeapon::SetSecondaryClipAmmo(int32 NewSecondaryClipAmmo)
{
	int32 OldSecondaryClipAmmo = SecondaryClipAmmo;
	SecondaryClipAmmo = NewSecondaryClipAmmo;
	OnSecondaryClipAmmoChanged.Broadcast(OldSecondaryClipAmmo, SecondaryClipAmmo);
}

void AGSWeapon::SetMaxSecondaryClipAmmo(int32 NewMaxSecondaryClipAmmo)
{
	int32 OldMaxSecondaryClipAmmo = MaxSecondaryClipAmmo;
	MaxSecondaryClipAmmo = NewMaxSecondaryClipAmmo;
	OnMaxSecondaryClipAmmoChanged.Broadcast(OldMaxSecondaryClipAmmo, MaxSecondaryClipAmmo);
}

TSubclassOf<UGSHUDReticle> AGSWeapon::GetPrimaryHUDReticleClass() const
{
	return PrimaryHUDReticleClass;
}

bool AGSWeapon::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

UAnimMontage* AGSWeapon::GetEquip1PMontage() const
{
	return Equip1PMontage;
}

UAnimMontage* AGSWeapon::GetEquip3PMontage() const
{
	return Equip3PMontage;
}

USoundCue* AGSWeapon::GetPickupSound() const
{
	return PickupSound;
}

FText AGSWeapon::GetDefaultStatusText() const
{
	return DefaultStatusText;
}

AGSGATA_LineTrace* AGSWeapon::GetLineTraceTargetActor()
{
	if (LineTraceTargetActor)
	{
		return LineTraceTargetActor;
	}

	LineTraceTargetActor = GetWorld()->SpawnActor<AGSGATA_LineTrace>();
	LineTraceTargetActor->SetOwner(this);
	return LineTraceTargetActor;
}

AGSGATA_LineTraceWithBloom* AGSWeapon::GetLineTraceWithBloomTargetActor()
{
	if (LineTraceWithBloomTargetActor)
	{
		return LineTraceWithBloomTargetActor;
	}

	LineTraceWithBloomTargetActor = GetWorld()->SpawnActor<AGSGATA_LineTraceWithBloom>();
	LineTraceWithBloomTargetActor->SetOwner(this);
	return LineTraceWithBloomTargetActor;
}

AGSGATA_SphereTrace* AGSWeapon::GetSphereTraceTargetActor()
{
	if (SphereTraceTargetActor)
	{
		return SphereTraceTargetActor;
	}

	SphereTraceTargetActor = GetWorld()->SpawnActor<AGSGATA_SphereTrace>();
	SphereTraceTargetActor->SetOwner(this);
	return SphereTraceTargetActor;
}

FVector AGSWeapon::GetADSOffset() const
{
	return ADSOffset;
}

void AGSWeapon::BeginPlay()
{
	ResetWeapon();

	if (!OwningCharacter && bSpawnWithCollision)
	{
		// Spawned into the world without an owner, enable collision as we are in pickup mode
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	Super::BeginPlay();
}

void AGSWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (LineTraceTargetActor)
	{
		LineTraceTargetActor->Destroy();
	}

	if (SphereTraceTargetActor)
	{
		SphereTraceTargetActor->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

void AGSWeapon::PickUpOnTouch(AGSHeroCharacter* InCharacter)
{
	if (!InCharacter || !InCharacter->IsAlive() || !InCharacter->GetAbilitySystemComponent() || InCharacter->GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(RestrictedPickupTags))
	{
		return;
	}

	if (InCharacter->AddWeaponToInventory(this, true) && OwningCharacter->IsInFirstPersonPerspective())
	{
		WeaponMesh3P->CastShadow = false;
		WeaponMesh3P->SetVisibility(true, true);
		WeaponMesh3P->SetVisibility(false, true);
	}
}

void AGSWeapon::OnRep_PrimaryClipAmmo(int32 OldPrimaryClipAmmo)
{
	OnPrimaryClipAmmoChanged.Broadcast(OldPrimaryClipAmmo, PrimaryClipAmmo);
}

void AGSWeapon::OnRep_MaxPrimaryClipAmmo(int32 OldMaxPrimaryClipAmmo)
{
	OnMaxPrimaryClipAmmoChanged.Broadcast(OldMaxPrimaryClipAmmo, MaxPrimaryClipAmmo);
}

void AGSWeapon::OnRep_SecondaryClipAmmo(int32 OldSecondaryClipAmmo)
{
	OnSecondaryClipAmmoChanged.Broadcast(OldSecondaryClipAmmo, SecondaryClipAmmo);
}

void AGSWeapon::OnRep_MaxSecondaryClipAmmo(int32 OldMaxSecondaryClipAmmo)
{
	OnMaxSecondaryClipAmmoChanged.Broadcast(OldMaxSecondaryClipAmmo, MaxSecondaryClipAmmo);
}

float AGSWeapon::SampleRecoilDirection(float x)
{
	// is it better to use a curve asset or just sample it directly from the function? idk man
	return FMath::Sin((x + 5.f) * (2.f * PI / 20.f)) * (100.f - x);
}

void AGSWeapon::StartRecoil()
{
	InitialRecoilPitchForce = BaseRecoilPitchForce;
	InitialRecoilYawForce = BaseRecoilYawForce;

	if (FireMode == FullAutoFireMode)
	{
		CurrentADSHeat = OwningCharacter->GetADSAlpha() > 0.f ? CurrentADSHeat + 1.f : 0.f;
		float ADSHeatModifier = FMath::Clamp(CurrentADSHeat / MaxADSHeat, 0.f, ADSHeatModifierMax);
		InitialRecoilPitchForce *= 1.f - ADSHeatModifier;
		InitialRecoilYawForce *= 1.f - ADSHeatModifier;
	}

	RecoilPitchVelocity = InitialRecoilPitchForce;
	RecoilPitchDamping = RecoilPitchVelocity / 0.1f;

	std::random_device rd;
	std::mt19937 gen(rd());
	float directionStat = SampleRecoilDirection(RecoilStat);
	float directionScaleModifier = directionStat / 100.f;
	float stddev = InitialRecoilYawForce * (1.f - RecoilStat / 100.f);

	std::normal_distribution<float> d(InitialRecoilYawForce * directionScaleModifier, stddev);
	RecoilYawVelocity = d(gen);
	RecoilYawDamping = (RecoilYawVelocity * -1.f) / 0.1f;

	bIsRecoilActive = true;
}

void AGSWeapon::StartRecoilRecovery()
{
	bIsRecoilPitchRecoveryActive = true;
	bIsRecoilYawRecoveryActive = true;
}

void AGSWeapon::IncrementSpread()
{
	float maxSpread = FMath::Lerp(TargetingSpreadMax, TargetingSpreadMaxADS, OwningCharacter->GetADSAlpha());
	float spreadIncrement = FMath::Lerp(TargetingSpreadIncrement, TargetingSpreadIncrement * SpreadIncrementADSMod, OwningCharacter->GetADSAlpha());
	CurrentTargetingSpread = FMath::Min(maxSpread, CurrentTargetingSpread + spreadIncrement);
}

float AGSWeapon::GetCurrentSpread() const
{
	return BaseSpread + CurrentTargetingSpread - FMath::Lerp(0.f, BaseSpread, OwningCharacter->GetADSAlpha());
}
