# 2600.emu

Atari 2600 emulator for multiple platforms, part of the EX Emulators suite.

## Overview

2600.emu is a cross-platform Atari 2600 (Video Computer System) emulator based on the Stella emulator core. It provides accurate emulation of the classic 1977 gaming console with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Atari 2600** (also known as Atari VCS)
  - NTSC, PAL, SECAM video systems
  - NTSC50, PAL60, SECAM60 variants

### Supported File Formats

- `.a26` - Atari 2600 ROM files
- `.bin` - Binary ROM dumps

## Features

### Emulation Features

- **Extensive Cartridge Support**: 54+ mapper types including:
  - Standard formats (2K, 4K, F4, F6, F8)
  - Enhanced mappers (DPC, DPC+, CDF, BUS, ELF)
  - Special hardware (Supercharger AR, AtariVox, SaveKey)
  - Homebrew formats (3E, 3E+, 3EX, JANE)

- **Controller Support**:
  - Joystick (standard single-button)
  - Genesis Gamepad (3-button)
  - Booster Grip (2-button)
  - Paddles (analog and digital control)
  - Keyboard (keypad input)
  - Auto-detection from ROM database

- **Video Features**:
  - TV phosphor simulation (adjustable blend 70-100%)
  - Multiple video system modes (Auto, NTSC, PAL, SECAM)
  - 4:3 aspect ratio with configurable options
  - NTSC filter support

- **Audio Features**:
  - High-quality audio resampling (Low/High/Ultra via Lanczos)
  - Configurable sample rates
  - High-pass filter support

- **Advanced Features**:
  - Save state support (.sta files)
  - Console switch emulation (Select, Reset, Color/B&W)
  - Difficulty switch toggling
  - Rewind functionality
  - ROM properties database (MD5-based)
  - ELF format for modern homebrew

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

cd 2600.emu
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

**No BIOS files required.** The Atari 2600 did not use a BIOS. All standard Atari 2600 ROM files work directly without additional system files.

**Note**: The Supercharger (AR) cartridge type uses an internal dummy BIOS built into the emulator.

## Resources

### Upstream Projects

- **Stella Emulator**
  - Website: https://stella-emu.github.io/
  - GitHub: https://github.com/stella-emu/stella
  - License: GPL-3.0

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

2600.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **2600.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Stella Emulator Core**: Copyright © The Stella Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
