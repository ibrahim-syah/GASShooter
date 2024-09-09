# GASShooter
This is a fork of [this](https://github.com/shadowfinderstudios/GASShooter) repo which itself is a fork of [this](https://github.com/tranek/GASShooter) repo. I couldn't fork off of it directly because of github LFS or something idk.

## Additions by Ibrahim:
I just modded here and there while I'm learning Unreal Engine. The original [author](https://github.com/tranek) has made a really good base for a shooter game, so I decided to start from here.
My understanding of GAS and Multiplayer/Networking/Replication is still very little, my main objective at the moment is too see if I could have decent base for a generic shooter game.

Below are the assets and tutorial I used. All the assets used in this project are freely available from the internet, this project is meant for educational purpose only. 
All credits belong to their respected creator (idk how to write disclaimer or licences or anything, so if you are one of the authors
and aren't comfortable with your work being distributed here, feel free to let me know so I can remove this repo).

- [Lyra](https://www.unrealengine.com/marketplace/en-US/product/lyra) Art Assets (Weapon mesh, animations, SFX, VFX)
- [Octahedron](https://www.youtube.com/playlist?list=PLxYB4IVFm8q6tf3boC1Nm4A8AB2RMCpXn) Procedural FP Animation System, Cannon FP Animations and Cannon Scope Sight
- [ALAN_Dev](https://www.youtube.com/watch?v=4ss29yFcAZc) Radial Damage Indicator
- [UntitledProjectX](https://www.youtube.com/watch?v=pmIRS8C_tFU) Hit Marker and Kill Marker
- [Jonatan Isaksson](https://www.youtube.com/watch?v=8sP71Y0K6uU&t=623s) Dynamic Healthbar
- [SneakyKittyGameDev](https://www.youtube.com/playlist?list=PLnHeglBaPYu--A7jTjNrgWdmYkR_kpZYu) Reflex Sight Asset
- [ArtofSith](https://www.artofsith.com/dynamic-reticles) Dynamic Reticles
- [TastyTony](https://sketchfab.com/TastyTony/collections) AssaultRifle and RSh-12 Models
- [Kenney](https://kenney.nl/) Crosshair and Skull Sprite
- [bulletmunchr](https://www.youtube.com/watch?v=q6DN0geJjkM&t=288s) Parallax Crosshair
- [Ramsterz](https://www.youtube.com/watch?v=UORC3YjlbZQ) Takedown Animation
- [Mixamo](https://www.mixamo.com/#/) Dizzy and Knocked Down Animation
- [Infima Games](https://infimagames.gumroad.com/l/fps-tutorial-source-files?layout=profile) Assault Rifle FP Animation (from the free UE5 FPS Tutorial)

## Fixes by Shadowfinder

* Upgraded to Unreal 5.2.1
* Fixed picking up items.
* Removed extra branches for double checking door state.
* Blood splatter shows up when third person perspective is enabled.
* Removed deprecated input system and replaced it with the EnhancedInputSystem.

## Introduction

GASShooter is an advanced FPS/TPS Sample Project for Unreal Engine 5's GameplayAbilitySystem (GAS) plugin. This is a sister project to the [GASDocumentation](https://github.com/tranek/GASDocumentation) and information about the techniques demonstrated here will be discussed in detail in the README there.

This is not production-ready code but a starting point for evaluating different techniques in GAS relating to using weapons. TargetActors with persistent hit results and ReticleActors particularly do a lot of code on `Tick()`.

Assets included come from Epic Games' ShooterGame learning project, Epic Games' Infinity Blade assets, or made by myself.

GASShooter is current with **Unreal Engine 5.21**.

| Keybind             | Action                                                      |
| ------------------- | ----------------------------------------------------------- |
| T                   | Toggles between first and third person.                     |
| Left Mouse Button   | Activates the weapon's primary ability. Confirms targeting. |
| Middle Mouse Button | Activates the weapon's alternate ability.                   |
| Right Mouse Button  | Activates the weapon's secondary ability.                   |
| Mouse Wheel Up      | Swaps to next weapon in inventory.                          |
| Mouse Wheel Down    | Swaps to previous weapon in inventory.                      |
| R                   | Reloads the weapon.                                         |
| Left Ctrl           | Cancels targeting.                                          |
| Left Shift          | Sprint.                                                     |
| E                   | Interact with interactable objects.                         |

| Console Command | Action                  |
| --------------- | ----------------------- |
| `kill`          | Kills the local player. |

The Hero character does have mana but no abilities currently use it. This project's inception started when the new BioShock was announced and the idea was to include BioShock-like upgradeable abilities. That made the scope too large, but it is something that may be revisited in the future.

Secondary ammo is not used. It would be used for things like rifle grenades.

## Concepts covered

* [Ability Batching](https://github.com/tranek/GASDocumentation#concepts-ga-batching)
* Equippable weapons that grant abilities
* Predicting weapon switching
* [Weapon ammo](https://github.com/tranek/GASDocumentation#concepts-as-design-itemattributes)
* Simple weapon inventory
* Headshot bonus damage
* [Reusable, custom TargetActors](https://github.com/tranek/GASDocumentation#concepts-targeting-actors)
* [GameplayAbilityWorldReticles](https://github.com/tranek/GASDocumentation#concepts-targeting-reticles)
* Play replicated montages on multiple Skeletal Mesh Components **belonging to the AvatarActor** in an ability
* [Subclassing `FGameplayEffectContext`](https://github.com/tranek/GASDocumentation#concepts-ge-context) to send additional information to GameplayCues
* Character shield that drains before health is removed by damage
* Item pickups
* Single button interaction system. Press or Hold 'E' to interact with interactable objects including player reviving, a weapon chest, and a sliding door.

This project does not show predicting projectiles. I refer you to the Unreal Tournament source code for how to do that using a fake projectile on the owning client.

| Weapon          | Primary Ability (Left Mouse Button)                  | Secondary Ability (Right Mouse Button)                                                                     | Alternate Ability (Middle Mouse Button)                     |
| --------------- | ---------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- |
| Rifle           | Fire hitscan bullets based on the current fire mode. | Aim down sights, reduces firing spread.                                                                    | Changes fire modes between full auto, semi auto, and burst. |
| Rocket Launcher | Fire a rocket.                                       | Aim down sights. Starts lock-on targeting for homing rockets. Press LMB to fire homing rockets at targets. | None                                                        |
| Shotgun         | Fire hitscan pellets based on the current fire mode. | Aim down sights, reduces firing spread for pellets.                                                        | Changes fire modes between semi auto and full auto.         |

## Acknowledgements

[KaosSpectrum](https://github.com/KaosSpectrum) provided significant contributions to figuring out how the ability batching system works and general feedback. Check out his game development [blog](https://www.thegames.dev/).