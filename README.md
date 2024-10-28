# Physics Hit React <img align="right" width=128, height=128 src="https://github.com/Vaei/PhysicsHitReact/blob/main/Resources/Icon128.png">

> [!IMPORTANT]
> **Procedural physics-driven hit reactions for skeletal meshes!**
> <br>High levels of customization and extendability, covering many use-cases
> <br>Exceptionally easy to use
> <br>Fully extendable in Blueprint
> <br>And its **Free!**

> [!TIP]
> Suitable for both singleplayer and multiplayer games

## Why Make a Hit React Plugin?
Solutions available online suffer from a myriad of limitations.

The primary limitation is that they don't have support for multiple overlapping hit reacts.

They don't throttle rapid re-application, and they don't handle rapid re-application either, so when hits occur in quick succession the results are visually jarring.

Furthermore, they allow parent bones to overwrite blend weights and other physic state properties on child bones.

They also don't handle resimulating an already simulating bone well, if we have interpolated the blend weight of the hand to max, and now we're interpolating back to 0, what happens? Do we ignore it and apply an impulse, or do we reset to 0 and incur a snap? PhysicsHitReact handles this with a concept of decay, which is smoothly interpolated and applied on each hit.

The other major issue is simply the lack of real-world use-cases, even if you have a procedural hit react occurring for a single purpose, what happens if the character pulls out a rifle, surely we don't want those arms to simulate? PhysicsHitReact allows disabling physics on a per-bone basis. Simply tell it to use the NoArms profile!

PhysicsHitReact overcomes all of these limitations, resulting in a highly stable and robust framework that functions in the production of a real game.

## Features
### Impulses
Supports Linear, Angular, and Radial impulses

Supports applying impulses to bones separate from the simulated bone

### Per-Bone Tuning
Supports disabling physics or using custom blend weights on child bones of the simulated bone

This means we can disable physics only on the arms for our character who is aiming a rifle!

### Profiles
Supports extendable profiles, simply specify the profile you want to use when triggering a Hit React

TODO

## How to Use
> [!IMPORTANT]
> [Read the Wiki to Learn How to use Hit React](https://github.com/Vaei/PhysicsHitReact/wiki)

## Example

TODO

## Changelog

### 1.4.0-beta
* Completely rebuilt the interpolation to better handle multiple states
* Completely rebuilt the global toggle
* All interpolation values are time based instead of rate based now
* Decay works properly, full support for Hold state
* Bones are sorted so child bones are processed last, allowing them to maintain their own blend weights
* Added new built-in blend profiles
	* Added TakeShot Unarmed/Armed profiles designed for rapid reapplication
	* Added TakeMelee Unarmed/Armed profiles designed for slower reapplication

### 1.3.0-beta
* Introduced the concept of interpolation decay, to handle reapplication of in-progress hit reacts
	* Getting hit repeatedly can now smoothly rewind the blend system partially!
	* WIP system that will become fully fledged in 1.4.0
* Overhauled application of hit reacts based on real-world usage
* Overhauled default profiles for great starting point parameters
	* Added more profiles
* Added PreActivate as a convenient point to cast and cache the owner in derived UHitReactComponent
* Add BlacklistBones and RemapBones for both simulated and impulse bones
* Added intensive data validation to UHitReactComponent to ensure coherent data
	* This is only really intended to avoid user-error resulting in undesired outcomes, hit react itself doesn't need it
* Improved debugging capability with p.HitReact.Debug.BlendWeights
* Fixed bug with cooldowns not applying
* Fixed bug with collision settings not updating
* Fixed edge case where completed hit reacts were not removed

### 1.2.1-beta
* Added global disable CVar

### 1.2.0-beta
* Restructure and unify properties and parameters based on tested use-cases
	* Introduce FHitReactApplyParams for passing around the required data to apply a hit react
	* Update UHitReactComponent::HitReact() to use this
* Expose the blend params -- oops
* Refactor properties>params for consistency
* Add net serialization to FHitReactImpulseParams and FHitReactImpulseWorldParams and all contained types
* Add BlueprintCosmetic where appropriate

### 1.1.2-beta
* Improve tick settings
* Improve dedi server handling
* Add PhysicsHitReactEditor module
* Add details customization to hide properties that shouldn't be touched
	* Tick in particular, because the system must handle it internally

### 1.1.1-beta
* Default profile if none provided to HitReact()
* Separate transient data from ImpulseParams into new WorldSpaceParams
* Improved failure debug logging
* Added check for physics asset

### 1.1.0-beta
* Add support to include GameplayAbilities as optional plugin
* Add support for ability tags to disable/enable the system
* Fix missing null check for optional PhysicalAnimationComponent

### 1.0.0-beta
* Initial Release
