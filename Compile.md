# Compiling Thinker

This guide focuses on two supported workflows:
- Using CMakePresets.json
- Using the Code::Blocks .cbp project files

## Using CMakePresets.json

Presets are defined in `CMakePresets.json` and configure builds under `build/<config>`.

Available presets:
- Unix Makefiles: `debug`, `develop`, `release`
- Ninja: `ninja-debug`, `ninja-develop`, `ninja-release`
- Code::Blocks - Unix Makefiles (generates a .cbp): `cb-debug`, `cb-develop`, `cb-release`

### Examples
1. Configure (choose one generator):
   - `cmake --preset debug`
   - `cmake --preset ninja-develop`
   - `cmake --preset cb-release`
2. Build
   - `cmake --build --preset debug` (library and launcher):
   - `cmake --build --preset develop --target thinkerlib` (only library)
   - `cmake --build --preset release --target thinker` (only launcher)

## Using Code::Blocks (.cbp)

Two options:
1) Open the provided projects directly:
   - `thinker.cbp` (builds the DLL)
   - `thinkerlaunch.cbp` (builds the launcher EXE)
2) Generate a Code::Blocks project via presets and open it:
   - `cmake --preset cb-debug`
   - `cmake --preset cb-develop`
   - `cmake --preset cb-release`
   * When using the Code::Blocks presets, open the generated `.cbp` from `build/<config>/`. 
