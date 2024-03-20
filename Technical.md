
Thinker command line options
============================

    -smac       Activate SMAC-in-SMACX mod. Implies smac_only=1 config option.
    -native     Start game in fullscreen mode using the native desktop resolution.
    -screen     Start game in fullscreen mode using the resolution set in mod config.
    -windowed   Start game in borderless windowed mode using the resolution set in mod config.


Thinker keyboard shortcuts
==========================

These shortcuts are always available whether debug build is available or not.

    Alt+T       Show Thinker's version and options menu (use Alt+H when PRACX is enabled).
    Alt+R       Toggle tile information update under cursor if smooth_scrolling is enabled.
    Alt+O       Enter a new value for random map generator when scenario editor mode is active.
    Alt+Enter   Switch between fullscreen and windowed mode unless desktop resolution is used in fullscreen.

These developer mode shortcuts are only available if Thinker is using debug build.
Some of these options require extended debug mode.

    Alt+D   Toggle extended debug mode.
    Alt+M   Toggle verbose logging mode on debug.txt.
    Alt+V   Iterate various TileSearch instances for the selected map tile.
    Alt+F   Iterate various move_upkeep overlays.
    Alt+Y   Toggle faction diplomacy treaty matrix display.
    Alt+X   Run pathfinding between two previous map tiles.
    Alt+Z   Print debug information for the selected map tile.
    Shift+4 Toggle world site priority display for the legacy AI.
    Shift+5 Toggle goals overlay display on the map.
    Shift+* Toggle faction border display on the map.

As a general troubleshooting feature, all Thinker builds also include a custom crash handler that
writes output to `debug.txt` in the game folder if the game happens to crash for any reason.


Tech tree visualization
=======================
As an additional feature, this repository includes a script written in Python 3 that can be used to visualize
any custom tech trees defined in alphax.txt. Techs are strictly aligned based on their level in the output
to make it easier to visualize the research order. To use the script, first install dependencies.

    pip install networkx matplotlib

Then plot any possible tech tree defined in alphax.txt to a png file.

    ./tools/techvisual.py -f techs.png /path/to/alphax.txt


Compiling Thinker
=================
Install dependencies to compile Thinker on Windows using CodeBlocks:

* [CodeBlocks](https://www.codeblocks.org/downloads/)
* [MinGW toolkit](https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev2/i686-12.2.0-release-posix-dwarf-msvcrt-rt_v10-rev2.7z)

To avoid compiling issues, do not use the older MinGW toolkit version bundled with CodeBlocks.
GCC versions older than 8.1.0 are not supported. After installing MinGW, new paths need to be
entered in Settings > Compiler > Toolchain Executables for the default GNU GCC Compiler profile.

Another way to build Thinker on Linux platforms is to convert the CodeBlocks project file into
actual Makefiles and then compile them using g++-mingw-w64. A build script that does all these
steps is provided in `tools/compile` folder.

1. First install dependencies: `apt install build-essential g++-mingw-w64-i686`
2. Clone [cbp2make](https://github.com/dmpas/cbp2make) and compile it from sources.
3. Install `cbp2make` binary on a folder included in the environment PATH.
4. Build Thinker: `./tools/compile/mingw-compile.sh`
5. Get compiled binaries from: `build/bin/`


Details for patch startup method
================================
Thinker's launcher program thinker.exe requires the original terranx.exe (game version 2.0) provided
by GOG in the same folder, (3 084 288 bytes, SHA-1 checksum: 4b19c1fe3266b5ebc4305cd182ed6e864e3a1c4a).
Using other official game binaries of the same version should also work. If an incompatible,
modified binary is found, the mod will display an error message and close the game.

Thinker will not modify terranx.exe file on disk, so it can still be used to start an unpatched
version of the game. Instead the mod will apply all of the changes at startup by patching
the binary in memory. Also Scient's patch v2.0 changes are automatically loaded at startup.

Source file `src/patchdata.h` contains Scient's Patch v2.0 changes that are applied at game startup.
Using a separate script this file can be converted into IDA PatchByte commands that can be used to
transform the default terranx.exe into a patched version that will also load thinker.dll at startup.
When the unmodified GOG binary is loaded in IDA, use this script to create the IDA PatchByte commands:

    ./tools/idapatch.py < src/patchdata.h

At game startup, Windows will attempt to load thinker.dll from the same folder before the game starts.
Note that Windows will terminate the process if any of the dll dependencies from the import table
are not available. When thinker.dll is loading, it will attempt to patch the game binary image
residing in the same address space before the actual game can start.

This way it is not required to write any of the actual patches to the exe file, instead they can be
applied dynamically at runtime. Thinker also performs some sanity checks during patching, and it
will close the game process with an error message if any of the checks fail.


Patching OpenSMACX to work with Thinker
=======================================
Due to the simplicity of Thinker's loading method, [OpenSMACX](https://github.com/b-casey/OpenSMACX/)
or the default terranx.exe can be patched manually in a hex editor to also load Thinker at startup.

Note that this combination of patches is not fully tested and some issues may appear due
to the combination of these binaries. After patching, OpenSMACX binary will not start
unless Windows can also load thinker.dll from the same folder.

1. Open `terranx_opensmacx_v0.2.1.exe` (or latest release) in a hex editor.
2. Replace any string `SHELL32.dll` with `thinker.dll`
3. Replace any string `ShellExecuteA` with `ThinkerModule`
4. Move all exe and dll files to the same folder with Alpha Centauri.

Thinker should now start along with OpenSMACX. Thinker version should always be visible in the game
version menu (Ctrl+F4). If this is not the case, Thinker did not start correctly with the game.
Check if you are actually running the game binary with the imports patched as described above.


Detailed changes in Scient patch
================================
This list describes the changes in src/patchdata.h in more detail while referencing Details.md.

* Fix 1 & 15. 005A239D, 006185F8 -> probe attempting to mind control base
* Fix 2.  00506865, 00618623 -> intercept crash
* Fix 3.  0045C3A5, 005C3E7, 0045C510 (Design Workshop overflow)
* Fix 3.  00618A94 -> increasing VM alloc at start
* Fix 4.  0046D40A -> adding checks to right click airdrop
* Fix 4.  004D89A8, 00618637 -> extended handling of airdrop for right click
* Fix 5.  004CDA6C, 00618668 -> fixes patrol waypoint bug
* Fix 6.  0052646B -> repair bay bug
* Fix 7.  00598F80 -> removing check which ends turn for certain air units when entering base
* Fix 8.  00494FBD, 006186A9 -> hurry production exploit
* Fix 9.  0046E778 -> recenters on unexplorered city tiles when clicked
* Fix 10. 004BFE81, 0061868C -> poles crash
* Fix 11. 00497FC6, 004972C0, 006186B9 -> base ops changing infiltrated workers exploit (right/left click)
* Fix 12. 004D7EAC, 006185A0 -> shift-g "go to home base"
* Fix 13. 0045CE09, 0061867C -> resetting cursor when clicking unit selection
* Fix 14. 004CD136, 006186E0 -> artillery range
* Fix 15. 005A1788, 00618616 -> probe attempting to mind control unit
* Fix 16. 004E6FCC -> borehole nutrient problem
* Fix 17. 00683C8B -> Believers were using incorrect ambient sound string
* Fix 18 & 19. 0059F0B9 -> flag to look into more whether correct mechanic
* Fix 20. 0045FA5E, 006187F8 : disable useless cpu check, ForceOldVoxelAlgorithm=0 by default (use MMX voxel code) but read in ini value
* Fix 20 also? 006392DC, 0063E85B : empty patch data spaces -> former cpu check functions
* Fix 21. 00593590, 006186ED -> sealurk movement in fungus
* Fix 22. **Removed**
* Fix 23. 0052A9BC, 0052AAB1, 006186FF -> registry check complete install
* Fix 24. 0059606B, 00618718 -> amphib / transport / sea base
* Fix 25. 005BA128, 00618736 -> "Ascent to Transcendence" SMAX only
* Fix 26. 00595869, 0059706B -> probe stacks
* Fix 27. 005A3AEA, 005A3C50, 00618763 -> AI #FREEWHO using faction slot 0
* Fix 28. 00403BA9, 004BEF8E, 00697214 -> movlistx
* Fix 29. 005C88E9 -> Nessus Canyon
* Fix 30. 00502F5B, 006392DC -> attacking along road
* Fix 31. 0051E01D, 0051E049, 0051E06A, 0051E0AB, 0051E14A, 0051E175, 0051E196, 0051E262, 0051E324 : change offsets of duplicate const registry str to make room for new str table
* Fix 31. 005201BF, 00520364, 005204BC, 0068AD88 -> NEWRESOURCE/PETERSOUT
* Fix 32. 0042EC63, 0042E18F, 006187A2 -> ARMORDESC/REACTORDESC datalinks
* Fix 33. 0042A648, 0042CAEF, 006187BB -> UNITDESC4 (sea formers)
* Fix 34. 00500E84 -> PB attempting to defend 2nd time after PB shot down
* Fix 34. 004CE516, 006187CF -> TM defending 2nd time after shot down
* Fix 34. 004CE9DD, 006187DD -> FM defending 2nd time after shot down
* Fix 35. 004CEA4F, 004CECB4, 006187EB -> Fungal Missiles spawning MW in ocean tiles
* Fix 36. 0050B51E -> nerve gas message
* Fix 37. 00587245, 0058731B -> ability to set reactor in alpha/x.txt
* Fix 38. 004CF834, 004CF88C, 004D0080 -> modifying restrictions on adding pods to bases
* Fix 39. 00596139, 00597323, 0063E86B -> DISEMBARKFAIL
* Fix 40. 0051F439, 0051F491 -> Perihelion init date
* Fix 41. 004CEA2A, 004CEB42, 004CED04, 004CED40, 004CED7A, 0063E85B -> FM #FUNGMOTIZED
* Fix 42. 004D11C8, 0063932C -> rare crash on AI attempting to upgrade unit
* Fix 43. 004F14C0, 004F1E57, 004F1F1B, 004F3FCA, 004F7A34 -> #ABANDONBASE fix
* Fix 44. 005950BD -> movement pact sea bases
* Fix 45. 00523849, 01238F6 -> interlude 6/7 issue #1
* Fix 46. 00505FBA -> interlude 6 issue #2
* Fix 47. 0041CFF7 -> base #CHANGEFORMER
* Fix 48. 005856C3 -> retooling parsing
* Fix 49. 004E4748, 0063E898 -> FREEPROTO & skunkworks
* Fix 50. 00409F24, 00409B96 -> save/load queue exploit
* Fix 51. 004D41D7 -> aquifer loop
* Fix 52. 0054ACD6 -> diplo faction id

