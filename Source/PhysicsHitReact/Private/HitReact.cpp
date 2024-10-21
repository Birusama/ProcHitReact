﻿// Copyright (c) Jared Taylor. All Rights Reserved.


#include "HitReact.h"

#include "PhysicsEngine/PhysicalAnimationComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HitReact)


bool FHitReact::NeedsCollisionEnabled() const
{
	return Mesh->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics && Mesh->GetCollisionEnabled() != ECollisionEnabled::PhysicsOnly;
}

bool FHitReact::CanSimulate() const
{
	// Must have valid mesh and owner
	if (!Mesh || !IsValid(Mesh->GetOwner()))
	{
		return false;
	}

	// Must have an actual mesh assigned
	if (!Mesh->GetSkeletalMeshAsset())
	{
		return false;
	}

	// Bone must exist
	const FReferenceSkeleton& RefSkeleton = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	if (RefSkeleton.FindBoneIndex(BoneName) == INDEX_NONE)
	{
		return false;
	}

	// Bone must have a body instance
	if (!Mesh->GetBodyInstance(BoneName))
	{
#if !UE_BUILD_SHIPPING
		const FString ErrorString = FString::Printf(TEXT("HitReact: No body instance for bone %s : %s : %s"),
			*BoneName.ToString(), *Mesh->GetOwner()->GetName(), *Mesh->GetName());
		FMessageLog("PIE").Error(FText::FromString(ErrorString));
#endif
		return false;
	}

	// Dev note - We simply trust that UHitReactComponent::HitReact() is the only caller,
	// which gates triggering hit reacts on dedicated servers for us

	return true;
}

void FHitReact::CacheBoneParams(const FName& InBoneName)
{
	// Reset cache if bone name has changed
	if (BoneName != InBoneName)
	{
		bHasCachedBoneExists = false;
		BoneName = InBoneName;
	}
	
	// Already cached
	if (bHasCachedBoneExists)
	{
		return;
	}

	// Reset cache
	bCachedBoneExists = false;

	// Must have valid mesh
	if (!Mesh)
	{
		return;
	}
	
	// Must have an actual mesh assigned
	if (!Mesh->GetSkeletalMeshAsset())
	{
		return;
	}

	// Bone must exist
	const FReferenceSkeleton& RefSkeleton = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	if (RefSkeleton.FindBoneIndex(BoneName) == INDEX_NONE)
	{
		return;
	}

	bCachedBoneExists = true;
}

bool FHitReact::HitReact(USkeletalMeshComponent* InMesh, UPhysicalAnimationComponent* InPhysicalAnimation,
	const FName& InBoneName, bool bIncludeSelf, const FHitReactProfile* Profile,
	const FHitReactBoneApplyParams* ApplyParams, const FHitReactImpulseParams& ImpulseParams)
{
	// Throttle hit reacts to prevent rapid application
	if (LastHitReactTime >= 0.f)
    {
        const float TimeSinceLastHitReact = InMesh->GetWorld()->TimeSince(LastHitReactTime);
        if (ApplyParams->Cooldown > TimeSinceLastHitReact)
        {
            return false;
        }
    }

	PhysicalAnimation = InPhysicalAnimation;
	Mesh = InMesh;
	CacheBoneParams(InBoneName);

	CachedProfile = Profile;
	CachedBoneParams = ApplyParams;
	
	if (CanSimulate())
	{
		if (NeedsCollisionEnabled())
		{
			bCollisionEnabledChanged = true;
			DefaultCollisionEnabled = Mesh->GetCollisionEnabled();
			switch (DefaultCollisionEnabled)
			{
			case ECollisionEnabled::NoCollision:
			case ECollisionEnabled::ProbeOnly:
				Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
				break;
			case ECollisionEnabled::QueryOnly:
			case ECollisionEnabled::QueryAndProbe:
				Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				break;
			default: break;
			}
		}
		
		LastHitReactTime = InMesh->GetWorld()->GetTimeSeconds();

		// Reset physics state prior to reinitialization, if not active
		if (!PhysicsState.IsActive())
		{
			// Reinitialize physics state
			PhysicsState.Initialize(0.f);
			
			// Enable physics simulation
			bCachedIncludeSelf = bIncludeSelf;
			Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, true, bCachedIncludeSelf);
			SetAllBodiesBelowPhysicsBlendWeight(CachedBoneParams->MinBlendWeight);

			// Apply physical anim profile
			if (PhysicalAnimation && !CachedBoneParams->PhysicalAnimProfile.IsNone())
			{
				PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(BoneName, CachedBoneParams->PhysicalAnimProfile, bCachedIncludeSelf);
			}

			// Apply constraint profile
			if (!CachedBoneParams->ConstraintProfile.IsNone())
			{
				Mesh->SetConstraintProfileForAll(CachedBoneParams->ConstraintProfile);
			}
		}
		
		// // Reinitialize physics state
		// PhysicsState.Initialize(0.f);
		
		// Interp forward
		InterpDirection = EInterpDirection::Forward;

		// Retrieve impulse parameters
		const FHitReactLinearImpulseParams& LinearParams = ImpulseParams.LinearImpulse;
		const FHitReactAngularImpulseParams& AngularParams = ImpulseParams.AngularImpulse;
		const FHitReactRadialImpulseParams& RadialParams = ImpulseParams.RadialImpulse;

		// Linear impulse
		if (LinearParams.CanBeApplied())
		{
			// Calculate linear impulse
			FVector Linear = LinearParams.GetImpulse() * CachedBoneParams->LinearImpulseScalar;
			if (CachedBoneParams->MaxLinearImpulse > 0.f)
			{
				Linear = Linear.GetClampedToMaxSize(CachedBoneParams->MaxLinearImpulse);
			}

			// Apply linear impulse
			if (!Linear.IsNearlyZero())
			{
				Mesh->AddImpulse(Linear, BoneName, LinearParams.IsVelocityChange());
			}
		}

		// Angular impulse
		if (AngularParams.CanBeApplied())
		{
			// Calculate angular impulse
			FVector Angular = AngularParams.GetImpulse() * CachedBoneParams->AngularImpulseScalar;
			if (CachedBoneParams->MaxAngularImpulse > 0.f)
			{
				Angular = Angular.GetClampedToMaxSize(CachedBoneParams->MaxAngularImpulse);
			}

			// Apply Angular impulse
			if (!Angular.IsNearlyZero())
			{
				switch (AngularParams.AngularUnits)
				{
				case EHitReactUnits::Degrees:
					Mesh->AddAngularImpulseInDegrees(Angular, BoneName, AngularParams.IsVelocityChange());
					break;
				case EHitReactUnits::Radians:
					Mesh->AddAngularImpulseInRadians(Angular, BoneName, AngularParams.IsVelocityChange());
					break;
				}
			}
		}

		// Radial impulse
		if (RadialParams.CanBeApplied())
		{
			// Calculate Radial impulse
			float Radial = RadialParams.Impulse * CachedBoneParams->RadialImpulseScalar;
			if (CachedBoneParams->MaxRadialImpulse > 0.f)
			{
				Radial = FMath::Min<float>(Radial, CachedBoneParams->MaxRadialImpulse);
			}

			// Apply Radial impulse
			if (!FMath::IsNearlyZero(Radial))
			{
				Mesh->AddRadialImpulse(RadialParams.WorldLocation, RadialParams.Radius, RadialParams.Impulse,
					RadialParams.Falloff, RadialParams.IsVelocityChange());
			}
		}
		
		return true;
	}

	return false;
}

bool FHitReact::Update(float GlobalScalar, float DeltaTime)
{
	// Update physics state
	if (PhysicsState.IsActive() && Mesh)
	{
		// Hold if we have a delay set
		if (InterpDirection == EInterpDirection::Hold)
		{
			HoldTimeRemaining -= DeltaTime;
			if (HoldTimeRemaining <= 0.f)
			{
				InterpDirection = EInterpDirection::Reverse;
			}
		}
		else
		{
			// Interpolate physics blend weight
			const float Target = InterpDirection == EInterpDirection::Forward ? 1.f : 0.f;
			PhysicsState.Interpolate(Target, DeltaTime);

			// Handle completion
			if (PhysicsState.HasCompleted())
			{
				// Finalize the value
				PhysicsState.Finalize();
			
				// Toggle direction
				InterpDirection = InterpDirection == EInterpDirection::Forward ? EInterpDirection::Reverse : EInterpDirection::Forward;

				// Delay before reversing if we have a delay set
				if (InterpDirection == EInterpDirection::Reverse && CachedBoneParams && CachedBoneParams->HoldTime > 0.f)
				{
					InterpDirection = EInterpDirection::Hold;
					HoldTimeRemaining = CachedBoneParams->HoldTime;
				}

				// Reset physics state if we have gone back to the start
				if (InterpDirection == EInterpDirection::Forward)
				{
					// Reset physics state
					PhysicsState.Reset();

					// Finalize physics simulation
					Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, 0.f);
					Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, false, bCachedIncludeSelf);

					// Clear physical anim profile
					if (PhysicalAnimation && !CachedBoneParams->PhysicalAnimProfile.IsNone())
					{
						PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(BoneName, NAME_None, bCachedIncludeSelf);
					}

					// Clear constraint profile
					if (!CachedBoneParams->ConstraintProfile.IsNone())
					{
						Mesh->SetConstraintProfileForAll(NAME_None, true);
					}
					
					// Restore collision enabled state
					if (bCollisionEnabledChanged)
					{
						Mesh->SetCollisionEnabled(DefaultCollisionEnabled);
					}

					// Notify caller that we have completed, and can be removed
					return true;
				}
			}
		}

		// Update physics blend weight
		const float TargetBlendWeight = PhysicsState.GetInterpolatedValue() * GlobalScalar;
		const float BlendWeight = FMath::Clamp<float>(TargetBlendWeight, CachedBoneParams->MinBlendWeight, CachedBoneParams->MaxBlendWeight);
		SetAllBodiesBelowPhysicsBlendWeight(BlendWeight);
	}

	// Not completed
	return false;
}

void FHitReact::SetAllBodiesBelowPhysicsBlendWeight(float PhysicsBlendWeight) const
{
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, PhysicsBlendWeight, false, bCachedIncludeSelf);

	if (CachedProfile && CachedProfile->OverrideBoneParams.Num() > 0)
	{
		for (auto& ParamsItr : CachedProfile->OverrideBoneParams)
		{
			const FName& OverrideBoneName = ParamsItr.Key;
			const FHitReactBoneParams& Params = ParamsItr.Value;
			const float BlendWeight = FMath::Clamp<float>(PhysicsBlendWeight, Params.MinBlendWeight, Params.MaxBlendWeight);
			Mesh->SetAllBodiesBelowPhysicsBlendWeight(OverrideBoneName, BlendWeight, false, true);
			if (Params.bDisablePhysics)
			{
				Mesh->SetAllBodiesBelowSimulatePhysics(OverrideBoneName, false, Params.bIncludeSelf);
			}
		}
	}
}
