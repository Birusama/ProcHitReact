﻿// Copyright (c) Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HitReactPhysicsState.h"
#include "HitReactTags.h"
#include "HitReactParams.generated.h"


/**
 * Input data for applying a hit reaction
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactParams
{
	GENERATED_BODY()

	FHitReactParams()
		: ProfileToUse(FHitReactTags::HitReact_Profile_Default)
		, SimulatedBoneName(NAME_None)
	    , bIncludeSelf(true)
	{}

	FHitReactParams(const FGameplayTag& InProfileToUse, const FName& InBoneName, bool bInIncludeSelf)
		: ProfileToUse(InProfileToUse)
		, SimulatedBoneName(InBoneName)
		, bIncludeSelf(bInIncludeSelf)
	{}

	/** Profile to use when applying the hit react, if none supplied, HitReact.Profile.Default will be used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FGameplayTag ProfileToUse;

	/**
	 * Bone to apply the hit reaction to -- this bone gets simulated
	 * Note that the simulated bone must have a physics body assigned in the physics asset
	 *
	 * This bone will also receive the impulse if ImpulseBoneName is None
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FName SimulatedBoneName;

	/**
	 * Optional bone to apply the impulse to
	 * This differs from the bone that is HitReacted, as the impulse bone is the bone that will receive the impulse
	 * And the HitReact bone is the bone that will be simulated
	 *
	 * If None, the impulse will be applied to the simulated bone instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FName ImpulseBoneName;

	/** If false, exclude the bone itself and only simulate bones below it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	bool bIncludeSelf;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << ProfileToUse;
		Ar << SimulatedBoneName;
		Ar << ImpulseBoneName;
		Ar << bIncludeSelf;
		return !Ar.IsError();
	}

	operator bool() const { return IsValidToApply(); }
	bool IsValidToApply() const { return ProfileToUse.IsValid() && !SimulatedBoneName.IsNone(); }
	
	const FName& GetImpulseBoneName() const
	{
		return ImpulseBoneName.IsNone() ? SimulatedBoneName : ImpulseBoneName;
	}
};

template<>
struct TStructOpsTypeTraits<FHitReactParams> : public TStructOpsTypeTraitsBase2<FHitReactParams>
{
	enum
	{
		WithNetSerializer = true
	};
};

/**
 * Bone-specific parameters for hit reactions
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactBoneParams
{
	GENERATED_BODY()
	
	FHitReactBoneParams(bool bInDisablePhysics = false, float InMinBlendWeight = 0.f, float InMaxBlendWeight = 1.f)
		: bDisablePhysics(bInDisablePhysics)
		, MinBlendWeight(InMinBlendWeight)
		, MaxBlendWeight(InMaxBlendWeight)
	{}

	/**
	 * If true, disable physics on this bone
	 * This will prevent inheriting physics from parent bones, it is not the same as setting MaxBlendWeight to 0
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	bool bDisablePhysics;
	
	/** Minimum weight provided to physical animation (0 is disabled, 1 is full) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", UIMax="1", ClampMax="1", EditCondition="!bDisablePhysics", ForceUnits="Percent"))
	float MinBlendWeight;

	/** Maximum weight provided to physical animation (0 is disabled, 1 is full) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", UIMax="1", ClampMax="1", EditCondition="!bDisablePhysics", ForceUnits="Percent"))
	float MaxBlendWeight;
};

/**
 * Bone-specific parameters for hit reactions
 * Used to override default values on a per-bone basis
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactBoneParamsOverride : public FHitReactBoneParams
{
	GENERATED_BODY()
	
	FHitReactBoneParamsOverride(bool bInIncludeSelf = true, bool bInDisablePhysics = false, float InMinBlendWeight = 0.f, float InMaxBlendWeight = 1.f)
		: FHitReactBoneParams(bInDisablePhysics, InMinBlendWeight, InMaxBlendWeight)
		, bIncludeSelf(bInIncludeSelf)
	{}

	/** If false, exclude the bone itself and only simulate bones below it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	bool bIncludeSelf;
};

/**
 * Handles scaling and limiting the impulse applied to a bone
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactImpulseScalar
{
	GENERATED_BODY()

	FHitReactImpulseScalar(float InScalar = 1.f, float InMax = 0.f)
		: Scalar(InScalar)
		, Max(InMax)
	{}
		
	/**
	 * Scale the impulse by this amount
	 * @see MaxImpulseTaken will mitigate this value if it would otherwise exceed MaxImpulseTaken
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0"))
	float Scalar;

	/** Maximum impulse that can be applied to this bone (at a single time). 0 to disable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0"))
	float Max;
};

/**
 * Bone-specific parameters for applying hit reactions
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactBoneApplyParams
{
	GENERATED_BODY()

	FHitReactBoneApplyParams()
		: PhysicsBlendParams(0.18f, 0.3f, EAlphaBlendOption::HermiteCubic)
		, MaxBlendWeight(0.4f)
		, Cooldown(0.15f)
		, DecayExistingPhysics(0.05f)
		, bReinitializeExistingPhysics(false)
		, SubsequentImpulseScalars({ 0.9f, 0.7f, 0.5f, 0.35f })
		, PhysicalAnimProfile(NAME_None)
		, ConstraintProfile(NAME_None)
	{}

	/**
	 * Interpolation parameters for physical animation
	 * Modifying these will change how quickly the bone interpolates in and out
	 * It is the most important parameter for controlling the look of the hit reaction
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", UIMax="1", ClampMax="1"))
	FHitReactPhysicsStateParams PhysicsBlendParams;

	/** Maximum weight provided to physical animation (0 is disabled, 1 is full) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", UIMax="1", ClampMax="1", ForceUnits="Percent"))
	float MaxBlendWeight;

	/** Delay before applying another impulse to prevent rapid application */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", ForceUnits=s))
	float Cooldown;

	/**
	 * Decay time to apply to the bone when the hit is being reapplied
	 * This allows repeated hits to push back the physics blend weight
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0", UIMax="1", ForceUnits="s", EditCondition="!bReinitializeExistingPhysics"))
	float DecayExistingPhysics;

	/**
	 * If true, will reinitialize physics on this bone from 0, causing a snap
	 * This provides more reliable/predictable results, but can be visually jarring in some cases
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	bool bReinitializeExistingPhysics;

	/**
	 * Scale the impulse based on the number of times this bone has been hit prior to completing the hit react
	 * The first array element is the scalar for the first subsequent hit, and so on
	 * This is used to throttle the impulse applied to the bone as it is hit multiple times
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	TArray<float> SubsequentImpulseScalars;
	
	/**
	 * Scale the impulse by this amount
	 * @see MaxImpulseTaken will mitigate this value if it would otherwise exceed MaxImpulseTaken
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0"))
	FHitReactImpulseScalar LinearImpulseScalar;

	/**
	 * Scale the impulse by this amount
	 * @see MaxImpulseTaken will mitigate this value if it would otherwise exceed MaxImpulseTaken
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0"))
	FHitReactImpulseScalar AngularImpulseScalar;

	/**
	 * Scale the impulse by this amount
	 * @see MaxImpulseTaken will mitigate this value if it would otherwise exceed MaxImpulseTaken
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(UIMin="0", ClampMin="0"))
	FHitReactImpulseScalar RadialImpulseScalar;

	/**
	 * Physical animation profile to apply to this bone
	 * Requires a Physical Animation Component to exist on the owning actor
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	FName PhysicalAnimProfile;

	/**
	 * Constraint profile to apply to this bone
	 * This is applied to the physics asset on the mesh
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	FName ConstraintProfile;
};

USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactBoneMapping
{
	GENERATED_BODY()

	FHitReactBoneMapping()
	{}

	FHitReactBoneMapping(const TMap<FName, FName>& InRemapBones)
		: RemapBones(InRemapBones)
	{}

	FHitReactBoneMapping(const TArray<FName>& InBlacklistBones)
		: BlacklistBones(InBlacklistBones)
	{}
	
	FHitReactBoneMapping(const TMap<FName, FName>& InRemapBones, const TArray<FName>& InBlacklistBones)
		: RemapBones(InRemapBones)
		, BlacklistBones(InBlacklistBones)
	{}
	
	/**
	 * Remap bones to other bones
	 * Intended for preventing hit reacts on certain bones, by remapping them to a bone that works better
	 * This is used to prevent pelvis from simulating as it can move the entire mesh!
	 * And also, bones that do not have physics bodies, such as spine_01 in the default UE5 Manny/Quinn
	 *
	 * We might want to remap simulated hand_l to upperarm_l, and impulse upperarm_l to hand_l, so the whole arm
	 *	is simulated and the impulse gets applied to the hand
	 *
	 * We apply blacklist bones first, then remap bones, so remapping a blacklisted bone will have no effect
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	TMap<FName, FName> RemapBones;

	/**
	 * Blacklist bones from hit reacts
	 * HitReact will abort entirely when attempting to HitReact these bones
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	TArray<FName> BlacklistBones;
};

/**
 * Profile to use when applying HitReact
 * Contains parameters for default bones and optional overrides for specific bones
 * Referenced by GameplayTag when applying HitReact
 */
USTRUCT(BlueprintType)
struct PROCHITREACT_API FHitReactProfile
{
	GENERATED_BODY()

	FHitReactProfile()
		: SimulatedBoneMapping({
			{ FName("pelvis"), FName("spine_02") },
			{ FName("spine_01"), FName("spine_02") },
		},
		{ "root" })
	{}

	/**
	 * Default bone params to use when applying HitReact
	 * Ignored if the simulated bone is overridden in OverrideBoneApplyParams
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FHitReactBoneApplyParams DefaultBoneApplyParams;

	/**
	 * Override bone parameters for specific bones when applying HitReact
	 * If the simulated bone is found in this map, the parameters will be used instead of DefaultBoneApplyParams
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	TMap<FName, FHitReactBoneApplyParams> OverrideBoneApplyParams;

	/**
	 * Override bone parameters for specific bones
	 * Allows clamping of child bone blend weights and per-child disabling of physics
	 * Applies to all child bones of the specified bone
	 * 
	 * @warning Order is vitally important here - We apply to all children of the bone, so the parent must be defined first
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	TMap<FName, FHitReactBoneParamsOverride> OverrideBoneParams;

	/* Handle remapping and blacklisting bones */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FHitReactBoneMapping SimulatedBoneMapping;
	
	/* Handle remapping and blacklisting bones */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HitReact)
	FHitReactBoneMapping ImpulseBoneMapping;
};

/**
 * Convenience struct for storing built-in HitReact profiles
 * Keeps the profiles in one place for easy access, and keeps HitReactComponent ctor clean
 */
USTRUCT()
struct FHitReactBuiltInProfiles
{
	GENERATED_BODY()

	static TMap<FGameplayTag, FHitReactProfile> GetBuiltInProfiles();

	/** Default profile to use as a starting point */
	static FHitReactProfile BuiltInProfile_Default();

	/** Default profile - No arms */
	static FHitReactProfile BuiltInProfile_Default_NoArms();

	/** Default profile - No legs */
	static FHitReactProfile BuiltInProfile_Default_NoLegs();

	/** Default profile - No arms or legs */
	static FHitReactProfile BuiltInProfile_Default_NoLimbs();

	/** Getting hit by a weapon */
	static FHitReactProfile BuiltInProfile_TakeHit();

	/** Getting hit by a weapon - No arms */
	static FHitReactProfile BuiltInProfile_TakeHit_NoArms();

	/** Getting hit by a weapon - No legs */
	static FHitReactProfile BuiltInProfile_TakeHit_NoLegs();

	/** Getting hit by a weapon - No arms or legs */
	static FHitReactProfile BuiltInProfile_TakeHit_NoLimbs();

	/** Twitching when getting shot rapidly */
	static FHitReactProfile BuiltInProfile_Twitch();

	/** Twitching when getting shot rapidly - No arms */
	static FHitReactProfile BuiltInProfile_Twitch_NoArms();

	/** Twitching when getting shot rapidly - No legs */
	static FHitReactProfile BuiltInProfile_Twitch_NoLegs();

	/** Twitching when getting shot rapidly - No arms or legs */
	static FHitReactProfile BuiltInProfile_Twitch_NoLimbs();

	/* Walking into things like other Pawns */
	static FHitReactProfile BuiltInProfile_BumpPawn();

	/** Make the character as floppy as possible, primarily used for testing purposes */
	static FHitReactProfile BuiltInProfile_Flop();
};