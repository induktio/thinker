# Compiling Thinker

## Overview

CMake can generate project files for different IDEs and build systems. Choose the generator that matches your development environment.

## CLion

CLion has native CMake support and will automatically use the existing `CMakeLists.txt` and `.idea` folder configurations. Simply open the project directory in CLion.

## Code::Blocks

**On Windows with MinGW:**
```bash
cmake -G "CodeBlocks - MinGW Makefiles" -B build
```

Then open `build/thinker.cbp` in Code::Blocks.

## Visual Studio (Windows)

**Visual Studio 2022:**
```bash
cmake -G "Visual Studio 17 2022" -B build
```

Then open `build/thinker.sln` in Visual Studio.

## Other Build Systems

**Unix Makefiles:**
```bash
cmake -G "Unix Makefiles" -B build
```

**Ninja:**
```bash
cmake -G "Ninja" -B build
```

## Building from Command Line

After generating the build files:

```bash
cd build
make
```
