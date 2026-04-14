# Saturn.emu

Sega Saturn emulator for multiple platforms, part of the EX Emulators suite.

## Overview

Saturn.emu is a cross-platform Sega Saturn emulator based on the Mednafen emulation core. It provides accurate emulation of the Sega Saturn console with support for various input devices, cartridges, and special features.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Sega Saturn** - The main home console (NTSC and PAL regions)
- **ST-V (Sega Titan Video)** - Arcade system board based on Saturn hardware

### Supported File Formats

- `.cue` - Cue sheet (with associated .bin files)
- `.ccd` - CloneCD image format
- `.chd` - MAME compressed hard disk format
- `.toc` - CDRWin TOC format
- `.m3u` - Multi-disc playlist (for disc-swapping games)

## Features

### Emulation Features

- Full Saturn hardware emulation including:
  - Dual Hitachi SH-2 CPUs
  - VDP1 (sprite/polygon processor) and VDP2 (background/scroll processor)
  - SCU (System Control Unit) with DSP
  - SCSP sound processor (Yamaha)
  - CD-ROM drive emulation
  - SMPC (System Manager & Peripheral Control)

### Display Features

- Multiple resolution support (up to 704x576)
- PAL and NTSC video system support
- Deinterlacing modes (Bob deinterlacing)
- Configurable video line ranges for NTSC and PAL
- Horizontal overscan control
- Correct line aspect ratio option
- Widescreen mode support (4:3 to 16:9 scaling)

### Input Support

- Standard Gamepad - 6-button digital pad
- 3D Control Pad - Analog controller with trigger buttons
- Multitap - Up to 12 players simultaneously
- Mouse - Sega Saturn mouse
- Steering Wheel - Racing wheel controller
- Mission Stick - Flight stick controller
- Dual Mission Stick - Dual flight stick setup
- Light Gun - Virtua Gun support with pointer input
- Keyboard - Saturn keyboard (English/Japanese)

### Cartridge/Expansion Support

- 512K Backup RAM - Extended save memory
- 1M RAM - 1 megabyte RAM expansion
- 4M RAM - 4 megabyte RAM expansion (required for some games)
- King of Fighters '95 ROM - Special ROM cartridge
- Ultraman ROM - Special ROM cartridge
- Action Replay 4M Plus - Cheat device with RAM expansion
- 16M CS1 RAM - 16 megabyte RAM expansion
- ST-V - Sega Titan Video (arcade) mode

## Building

### Prerequisites

- **C++ Compiler**: GCC 16+, Clang 21+, or MSVC with C++20 support
- **Build Tools**: CMake 4.1+, GNU Make, pkg-config
- **Imagine SDK**: Cross-platform application framework
- **64-bit device required** for performance reasons

### Environment Setup

```bash
export IMAGINE_PATH=/path/to/imagine
export IMAGINE_SDK_PATH=$HOME/imagine-sdk
```

### Platform-Specific Builds

#### Linux

```bash
cd $IMAGINE_PATH
cmake --preset linux-x86_64
cmake --build build/linux-x86_64 --target install

cd Saturn.emu
cmake --preset linux-x86_64
cmake --build build/linux-x86_64
```

#### Android

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-android.sh install
```

Minimum SDK: 21 (Android 5.0), Supported architectures: arm64, x86_64

#### iOS

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-ios.sh install
```

Supported architecture: arm64

### Build Configuration

- **CMake**: Main build configuration in `CMakeLists.txt`
- **Platform Makefiles**: `linux.mk`, `android.mk`, `ios.mk`
- **Metadata**: `metadata/conf.mk` for package configuration

## BIOS Requirements

**Saturn.emu requires official Sega Saturn BIOS files to operate.**

### Required BIOS Files

You need the appropriate BIOS for your region:

**Japanese/Asian NTSC Region:**
- `sega1003.bin` (recommended)
- `sega_100.bin`
- `sega_101.bin`

**North American/European Region:**
- `sega_100a.bin` (recommended)
- `mpr-17933.bin`

**ST-V BIOS:**
- ST-V BIOS file (512KB) for arcade games

### BIOS Configuration

1. Navigate to **Options → File Paths**
2. Set **JP BIOS** path for Japanese games
3. Set **NA/EU BIOS** path for North American/European games
4. Set **KoF '95 ROM** path if playing King of Fighters '95
5. Set **Ultraman ROM** path if playing Ultraman games

### BIOS Specifications

- File size: Exactly 524,288 bytes (512KB)
- Format: Raw binary dump (.bin extension)
- The emulator performs SHA-256 hash validation to ensure BIOS integrity
- Save states are tied to specific BIOS files via SHA-256 hash

## Resources

### Upstream Projects

- **Mednafen**
  - Website: https://mednafen.github.io/
  - Copyright: Mednafen Team (2015-2025)
  - License: GPL-2.0

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

Saturn.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **Saturn.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Mednafen Saturn Core**: Copyright © Mednafen Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
