// Copyright (c) Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HitReactImpulseParams.generated.h"

UENUM(BlueprintType)
enum class EHitReactUnits : uint8
{
	Degrees,
	Radians
};

USTRUCT(BlueprintType)
struct PHYSICSHITREACT_API FHitReactImpulseParamsBase
{
	GENERATED_BODY()

	FHitReactImpulseParamsBase()
		: bApplyImpulse(false)
		, bFactorMass(false)
		, Impulse(1000.f)
	{}
	virtual ~FHitReactImpulseParamsBase() = default;

	/** If false, will not be applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	bool bApplyImpulse;

	/**	If true, impulse is taken as a change in velocity instead of an impulse (i.e. mass will have no effect). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse"))
	bool bFactorMass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse", UIMin="0", ClampMin="0"))
	float Impulse;

	bool IsVelocityChange() const { return !bFactorMass; }

	virtual bool CanBeApplied() const
	{
		return bApplyImpulse && Impulse > 0.f;
	}
};

USTRUCT(BlueprintType)
struct PHYSICSHITREACT_API FHitReactLinearImpulseParams : public FHitReactImpulseParamsBase
{
	GENERATED_BODY()

	FHitReactLinearImpulseParams()
		: WorldDirection(FVector::ZeroVector)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse"))
	FVector WorldDirection;

	virtual FVector GetImpulse() const
	{
		return WorldDirection * Impulse;
	}

	virtual bool CanBeApplied() const override final
	{
		return FHitReactImpulseParamsBase::CanBeApplied() && !WorldDirection.IsNearlyZero();
	}
};

USTRUCT(BlueprintType)
struct PHYSICSHITREACT_API FHitReactAngularImpulseParams : public FHitReactLinearImpulseParams
{
	GENERATED_BODY()

	FHitReactAngularImpulseParams()
		: AngularUnits(EHitReactUnits::Degrees)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	EHitReactUnits AngularUnits;
};

USTRUCT(BlueprintType)
struct PHYSICSHITREACT_API FHitReactRadialImpulseParams : public FHitReactImpulseParamsBase
{
	GENERATED_BODY()

	FHitReactRadialImpulseParams()
		: WorldLocation(FVector::ZeroVector)
		, Radius(50.f)
		, Falloff(RIF_Linear)
	{}

	/** World Location of the impulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse"))
	FVector WorldLocation;

	/** Radius of the impulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse", UIMin="0", ClampMin="0"))
	float Radius;

	/** How the strength of the impulse should fall off with distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics, meta=(EditCondition="bApplyImpulse"))
	TEnumAsByte<ERadialImpulseFalloff> Falloff;

	virtual bool CanBeApplied() const override final
	{
		return FHitReactImpulseParamsBase::CanBeApplied() && Radius > 0.f;
	}
};

USTRUCT(BlueprintType)
struct PHYSICSHITREACT_API FHitReactImpulseParams
{
	GENERATED_BODY()

	FHitReactImpulseParams()
	{
		LinearImpulse.bApplyImpulse = true;
	}

	FHitReactImpulseParams(const FHitReactLinearImpulseParams& InLinearImpulse, const FHitReactAngularImpulseParams& InAngularImpulse, const FHitReactRadialImpulseParams& InRadialImpulse)
		: LinearImpulse(InLinearImpulse)
		, AngularImpulse(InAngularImpulse)
		, RadialImpulse(InRadialImpulse)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	FHitReactLinearImpulseParams LinearImpulse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	FHitReactAngularImpulseParams AngularImpulse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	FHitReactRadialImpulseParams RadialImpulse;
};
