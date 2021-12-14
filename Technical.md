
Thinker command line options
============================

    -smac       Activate SMAC-in-SMACX mod. Implies smac_only=1 config option.
    -windowed   Start the game in borderless windowed mode. Uses resolution set in the config.


Thinker keyboard shortcuts
==========================

These shortcuts are available always, whether debug build is available or not.

    Alt-T   Show Thinker's version menu and options.
    Alt-R   Toggle tile information update under cursor if smooth_scrolling is enabled.

These developer mode shortcuts are only available if Thinker is using debug build. Some of these options require extended debug mode.

    Alt-D   Toggle extended debug mode.
    Alt-V   Iterate various TileSearch instances for the selected map tile.
    Alt-F   Iterate various move_upkeep overlays.
    Alt-Y   Toggle faction diplomacy treaty matrix display.
    Alt-X   Run pathfinding between two previous map tiles.
    Alt-Z   Print debug information for the selected map tile.
    Shift-4 Toggle world site priority display for the legacy AI.
    Shift-5 Toggle goals overlay display on the map.
    Shift-* Toggle faction border display on the map.

As a general troubleshooting feature, all Thinker builds also include a custom crash handler that writes output to `debug.txt` in the game folder if the game happens to crash for any reason.


Compiling Thinker
=================
The preferred and actively supported way to compile Thinker on Windows is to use [CodeBlocks](https://www.codeblocks.org/downloads/) and GCC with the included project file. GCC version 8.1.0 or later is recommended.

To avoid compiling issues, do not use the MinGW toolkit version bundled with CodeBlocks. Instead get the toolkit separately from the [MinGW-w64 project](https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/).

Another way to build Thinker on Linux platforms is to convert the CodeBlocks project files into actual Makefiles and then compile them using g++-mingw-w64. A build script that does all these steps is provided in `tools/compile` folder.

1. First install dependencies: `apt install build-essential g++-mingw-w64`
2. Clone [cbp2make](https://github.com/dmpas/cbp2make) and compile it from sources.
3. Install `cbp2make` binary on environment path.
4. Build Thinker: `./tools/compile/mingw-compile.sh`
5. Get compiled binaries from: `build/bin/`


Patches included in the release version
=======================================
Thinker's launcher program thinker.exe requires the original, unmodified terranx.exe provided by the GOG version in the same folder, (3 084 288 bytes, GOG version SHA-1: 4b19c1fe3266b5ebc4305cd182ed6e864e3a1c4a). Using other official game binaries of the same version may also work. If an incompatible, modified binary is found, the mod will display an error message and close the game.

Thinker will not modify terranx.exe file on disk, so it can still be used to start an unpatched version of the game. The mod will apply all of the changes at startup by patching the binary in memory. Also Scient's patch v2.0 changes are automatically loaded at startup.

This repo also contains an equivalent binary diff `terranx.dif` between the GOG binary and Thinker's version after all of the changes are patched in memory.

In the diff listing, the vast majority of changes relate to Scient's patch with the exception of these address ranges (38 lines total) that add thinker.dll to the dll import table. They will replace an existing unused entry that was left in the binary for unknown reason.

    00280A1E .. 00280A32
    002EF4FC .. 002EF516

After acquiring the diff listing, it is possible to use `idapatch.py` to convert it into PatchByte script commands that can used by IDA. When the unmodified GOG binary is loaded in IDA, entering all the PatchByte commands will transform it into a version that will also load thinker.dll at startup.

At game startup, Windows will attempt to load thinker.dll from the same folder before the game starts. Note that Windows will terminate the process if any of the dll dependencies from the import table are not available. When thinker.dll is loading, it will attempt to patch the game binary image residing in the same address space before the actual game can start. 

This way it is not required to write any of the actual patches to the exe file, instead they can be applied dynamically at runtime. Thinker also performs some sanity checks during patching, and it is designed to terminate the game process with an error message if any of the checks fail.


Patching OpenSMACX to work with Thinker
=======================================
Due to the simplicity of Thinker's loading method, [OpenSMACX](https://github.com/b-casey/OpenSMACX/) can be patched manually in a hex editor to also load Thinker at startup.

Note that this combination of patches is not fully tested and some glitches may appear in the game that are not present in the standard game binary. After patching, OpenSMACX binary will not start unless thinker.dll can also be loaded from the same folder by Windows.

1. Open `terranx_opensmacx_v0.2.1.exe` (or latest release) in a hex editor
2. Replace any string `SHELL32.dll` with `thinker.dll`
3. Replace any string `ShellExecuteA` with `ThinkerDecide`
4. Move all exe and dll files to the same folder with Alpha Centauri

Thinker should now start along with OpenSMACX. Thinker version should always be visible in the game version menu (Ctrl+F4). If this is not the case, Thinker did not start correctly with the game. Check if you are actually running the game binary with the imports patched as described above.

