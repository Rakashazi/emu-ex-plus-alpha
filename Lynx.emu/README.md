# Lynx.emu

Atari Lynx emulator for multiple platforms, part of the EX Emulators suite.

## Overview

Lynx.emu is a cross-platform Atari Lynx handheld console emulator based on components from the Handy emulator (originally by K. Wilkins) as integrated into the Mednafen multi-system emulator. It provides accurate emulation of the Atari Lynx hardware with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Atari Lynx** (1989)
  - Portable handheld gaming console
  - 8-bit 65C02 CPU @ 16 MHz
  - 160×102 pixel LCD display with 4096 color palette
  - Stereo sound with 4 channels
  - Supports both horizontal and vertical screen orientations

### Supported File Formats

- `.lnx` - Standard Lynx ROM format with header (most common)
- `.lyx` - Alternate Lynx ROM format
- `.o` - Homebrew Lynx executables (BS93 format)
- RAW dumps - Headerless ROMs (128KB, 256KB, or 512KB files)

## Features

### Emulation Features

- **Accurate emulation**: Based on Mednafen's Lynx emulation core using Handy components
- **Display rotation support**: Handles games designed for vertical orientation (left/right rotation)
  - Auto-detection based on ROM header
  - Manual override options available
  - D-pad input automatically rotates with screen orientation
- **Save states**: Full save state support with `.mca` file format
- **Audio options**: Configurable low-pass filter for smoother audio output
- **Original aspect ratio**: 80:51 (approximately 1.57:1)

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

cd Lynx.emu
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

**No BIOS files required.** The Atari Lynx boot ROM is **optional**. The emulator can run without it using HLE (High-Level Emulation) to simulate boot process.

### Optional Boot ROM

**File:** `lynxboot.img`
**Size:** 512 bytes

When configured, the BIOS provides authentic boot behavior and encrypted cart support. Without BIOS:
- Uses HLE to simulate boot process
- Automatically decrypts commercial game ROMs
- Fully compatible with homebrew software
- Most games work correctly without the BIOS

Configure the BIOS path through the emulator's GUI settings or configuration file (`LynxEmu.config`).

## Resources

### Upstream Projects

- **Mednafen**
  - Website: https://mednafen.github.io/
  - License: GPL-2.0
  - Lynx core based on Handy emulator

- **Handy Emulator Core**
  - Copyright: K. Wilkins (1996-1997, 2004)
  - License: zlib-style permissive license

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

Lynx.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **Lynx.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Handy Emulator Core**: Copyright © 1996-1997, 2004 K. Wilkins
- **Mednafen Integration**: Copyright © Mednafen Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
