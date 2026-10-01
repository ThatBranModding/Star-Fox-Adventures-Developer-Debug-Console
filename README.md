# SFA Developer Console

A developer console mod for **Star Fox Adventures** running through **Foxhollow**.

The mod provides an in-game developer console with area and boss teleports, infinite resource toggles and position information.

## Supported platforms

- Windows x64
- Linux x64
- macOS

## Requirements

- Foxhollow
- Star Fox Adventures running through Foxhollow

## Installation

Download either directly through the Foxhollow Launcher, or get the mod from GitHub Releases.

1. Download the most recent release ZIP.
2. Extract the `sfa-developer-console` folder into your Foxhollow Launcher `mods` directory.
3. Launch Star Fox Adventures through Foxhollow.
4. Press **F1** in-game to open the developer console.

## Features

- In-game console opened with **F1** and closed using either the same button or by pressing **ESC**
- Scrollable command history by using **PAGE UP**, **PAGE DOWN**, **HOME** and **END**.
- Named area teleports, supports act selection if supplied, else defaults to act 1 for all areas except krazoa palace which teleports you to its act 2.
- Boss and location teleports
- Infinite health, staff mana, and Tricky energy toggles


### General Commands

`help` - Lists all available commands.

`version` - Shows the current developer console version.

`position` - Shows your current position within the game map.

### Infinite Resource Commands

`infinite health` - Toggles infinite health on or off.

`infinite mana` - Toggles infinite staff mana on or off.

`infinite tricky` - Toggles infinite Tricky energy on or off.

`infinite status` - Shows the current state of the infinite resource toggles.

### Area Teleports

Use:

```text
teleport <location> [act]
```

Valid teleport inputs and their locations are:

```text
tth     - ThornTail Hollow
lfv     - LightFoot Village
crf     - CloudRunner Fortress
dim     - DarkIce Mines
cc      - Cape Claw
wc      - Walled City
dr      - Dragon Rock
mmp     - Moon Mountain Pass
ofp     - Ocean Force Point Temple
vfp     - Volcano Force Point Temple
im      - Ice Mountain
shw     - SnowHorn Wastes
kp      - Krazoa Palace
```

An act can be supplied for locations that support multiple acts, for example:

teleport kp 6

### Boss Teleports

Use:

```text
teleport boss <boss>
```

Valid boss teleport inputs and their locations are:

```text
galdon     - DarkIce Mines Galdon boss
race       - CloudRunner Fortress race map
redeye     - Walled City RedEye King boss
drakor     - Dragon Rock Drakor boss
andross    - Final boss Andross
```

## Notes

Developer teleports are intended for testing. A teleport destination will load correctly but wont load every adjacent connecting area between main maps. For example, the connecting map between Cape Claw and LightFoot Village may not be loaded when teleporting directly to Cape Claw. This is something I unfortunately cannot seem to get working seamlessly.

The Andross boss teleport first teleports the player to Krazoa Palace Act 6 to preload the objects required by the Andross boss map.

Infinite health affects both the ground playable character and the Arwing during their respective gameplay sections.

## AI Disclosure

AI tools were used during the development of this project to assist with code generation, debugging, reverse engineering, and development iteration. All functionality was tested and reviewed by myself before release.

## License

Licensed under the MIT License. See [LICENSE](LICENSE).