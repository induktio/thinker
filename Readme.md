
SMACX Thinker Mod
=================

Thinker is an AI improvement mod for Alpha Centauri: Alien Crossfire.
This mod increases the level of challenge in single player while providing many gameplay enhancements.
By patching the game to use an additional DLL, new functionality can be developed in C++. Most important features include:

* Vastly improved production/terraforming AI
* Improved code for AI social engineering choices
* AI combat units are rewritten to use more variety in tactics
* AI deploys colony pods much sooner instead of wandering on the map
* AI builds more crawlers and deploys them better
* More config options for many previously fixed settings in the game engine
* Many enhancements to random map generation/spawn locations
* Name labels for HQ bases are emphasized on the map
* Minimal changes to vanilla game mechanics
* Game binary includes [Scient's patches](Details.md)

This mod is tested to work with the [GOG version](https://www.gog.com/game/sid_meiers_alpha_centauri) of Alpha Centauri.
[See more information](Details.md) about the features and recommended settings.
It's strongly recommended to read Details.md as many features provided by the mod are not present in the vanilla game.
[Discuss here](https://github.com/induktio/thinker/discussions) about anything related to Thinker development.
Remember also to star and watch the repository to receive notifications about new updates.


Download
--------
This is the only place to download binary releases. See also the [Changelog](Changelog.md) for useful release notes.

* [Release versions](https://www.dropbox.com/sh/qsps5bhz8v020o9/AAAp6ioWxdo7vnG6Ity5W3o1a?dl=0&lst=)
* [Develop builds](https://www.dropbox.com/sh/qsps5bhz8v020o9/AADv-0D0-bPq22pgoAIcDRC3a/develop?dl=0&lst=)


Installation
------------
1. Extract the files to Alpha Centauri game folder. Alphax.txt changes are optional.
2. Check Changelog.md and Details.md for useful information.
3. Change configuration from thinker.ini or just use the defaults.
4. Start the game from thinker.exe.
5. The launcher requires original terranx.exe in the same folder but this file is not modified on disk. All the changes are applied at runtime.
6. Press ALT+T to open Thinker's options menu. Other option is to check that mod version/build date is visible in the game version menu (Ctrl+F4).
7. If none of those options display mod version, Thinker is incorrectly installed and not loaded.


Other mods
----------
* [SMAC-in-SMACX mod](Details.md#smac-in-smacx-mod) can be installed to play a game similar to original SMAC from the SMACX game binary while Thinker is enabled.
* [PRACX](https://github.com/DrazharLn/pracx) graphics enhancement patch can be used together with Thinker. For possible issues before installing see [compatibility with other mods](Details.md#compatibility-with-other-mods). For easiest installation, download [version 1.11 or later](https://github.com/DrazharLn/pracx/releases/).
* [OpenSMACX](https://github.com/b-casey/OpenSMACX) is a long-term project to decompile and create a full open source clone of SMACX.


Compiling
---------
For information on how to compile Thinker, see [Technical.md](Technical.md).


License
-------
This software is licensed under the GNU General Public License version 2, or (at your option) version 3 of the GPL. See [License.txt](License.txt) for details.

The original game assets are not covered by this license and remain property of Firaxis Games Inc and Electronic Arts Inc.

Sid Meier's Alpha Centauri and Sid Meier's Alien Crossfire is Copyright © 1997, 1998 by Firaxis Games Inc and Electronic Arts Inc.
