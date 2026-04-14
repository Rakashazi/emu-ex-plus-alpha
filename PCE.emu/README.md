# PCE.emu

TurboGrafx-16/PC Engine emulator for multiple platforms, part of the EX Emulators suite.

## Overview

PCE.emu is a cross-platform TurboGrafx-16/PC Engine emulator based on emulation cores from the Mednafen project. It provides accurate emulation of NEC's PC Engine family of game consoles with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **PC Engine / TurboGrafx-16** - The base console (1987/1989)
- **PC Engine SuperGrafx** - Enhanced version with improved graphics capabilities
- **TurboGrafx-CD / PC Engine CD-ROM²** - CD-ROM expansion system
- **PC Engine Arcade Card** - Memory expansion accessory for CD games

### Supported File Formats

**HuCard/ROM formats:**
- `.pce` - PC Engine ROM files
- `.sgx` - SuperGrafx ROM files

**CD formats:**
- `.cue` - CUE sheet (with associated BIN/ISO)
- `.toc` - TOC format
- `.ccd` - CloneCD format
- `.chd` - MAME compressed hard disk format

## Features

### Dual Emulation Cores

PCE.emu includes two Mednafen-based emulation cores:

1. **pce_fast** (Default) - Optimized for performance and general use, lower CPU usage, suitable for most games and devices
2. **pce** (Accurate) - Higher accuracy emulation, better compatibility with edge cases, recommended when accuracy is critical

**Note**: Save states are not compatible between cores.

### Video Features

- **Multiple Resolution Modes**: Handles 256, 341, and 512 pixel widths with automatic scaling
- **Configurable Visible Lines**: 11+224 (default), 18+224, 4+232, 3+239, 0+242 (maximum)
- **Sprite Limit Toggle**: Option to remove the original hardware sprite limit
- **Line Aspect Ratio Correction**: Corrects the aspect ratio based on visible scanlines
- **Dynamic Frame Rate**: ~59.82Hz (263 lines) and ~60.05Hz (262 lines)

### Audio Features

- **CD-DA Volume Control**: Adjustable CD audio volume (0-200%)
- **ADPCM Volume Control**: Adjustable ADPCM audio volume (0-200%)
- **ADPCM Low-pass Filter**: Optional audio filtering for improved sound quality
- **CD Speed Control**: Selectable CD access speed (1x, 2x, 4x, 8x) for faster loading

### Input Support

- **Up to 5 Players**: Full multitap support for 5 simultaneous players
- **6-button Gamepad Support**: Enable 6-button controllers (I, II, III, IV, V, VI buttons)
- **Turbo Button Support**: Built-in turbo functionality for all face buttons
- **Multiple Input Device Emulation**: Standard 2-button gamepad, 6-button gamepad, Mouse, Tsushin Keyboard

## Building

### Prerequisites

- **C++ Compiler**: GCC 16+, Clang 21+, or MSVC with C++20 support
- **Build Tools**: CMake 4.1+, GNU Make, pkg-config
- **Imagine SDK**: Cross-platform application framework

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

cd PCE.emu
cmake --preset linux-x86_64
cmake --build build/linux-x86_64
```

#### Android

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-android.sh install
```

#### iOS

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-ios.sh install
```

### Build Configuration

- **CMake**: Main build configuration in `CMakeLists.txt`
- **Platform Makefiles**: `linux.mk`, `android.mk`, `ios.mk`, `pandora.mk`
- **Metadata**: `metadata/conf.mk` for package configuration

## BIOS Requirements

### System Card (CD-ROM Games)

CD-ROM games require a PC Engine CD-ROM System Card BIOS file. You must provide one of the following:

**Supported System Cards:**
- System Card 1
- System Card 2
- System Card 3
- Games Express CD Card

**Setup:**
1. Obtain a valid System Card BIOS file (typically `syscard3.pce` or similar)
2. Configure the System Card path in the emulator settings: File Paths → System Card
3. Select your System Card BIOS file

**Note:** HuCard (cartridge) games do **not** require any BIOS files and can be loaded directly.

## Resources

### Upstream Projects

- **Mednafen**
  - Website: https://mednafen.github.io
  - License: GPL-2.0
  - Original emulation cores (pce and pce_fast modules)

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

PCE.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **PCE.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Mednafen Emulation Cores**: Copyright © Mednafen Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
