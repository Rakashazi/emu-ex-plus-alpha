# GBA.emu

Game Boy Advance emulator for multiple platforms, part of the EX Emulators suite.

## Overview

GBA.emu is a cross-platform Game Boy Advance emulator based on components from the VBA-M (VisualBoyAdvance-M) emulator. It provides accurate emulation of the Nintendo Game Boy Advance handheld console with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Nintendo Game Boy Advance (GBA)** - Standard GBA games
  - Display resolution: 240×160 pixels at approximately 59.7275 Hz
  - Native 3:2 aspect ratio

### Supported File Formats

- `.gba` - Game Boy Advance ROM files
- `.mb` - Multiboot ROM images

## Features

### Emulation Features

- **VBA-M Based Core**: Uses emulation components from VBA-M, a mature and accurate GBA emulator
- **Save Game Support**: Multiple save types with automatic detection
  - EEPROM (512 bytes / 8KB)
  - SRAM (32KB)
  - Flash (64KB / 128KB)
  - EEPROM + Sensor
  - Manual save type override available if auto-detection fails
- **Real-Time Clock (RTC) Emulation**: Automatic detection and support for games that use RTC (e.g., Pokémon Ruby/Sapphire/Emerald)
- **State Save/Load**: Full save state support with gzip compression
- **Cheat Code Support**: Built-in cheat system compatible with VBA-M cheat formats

### Special Features

- **BIOS Support**: Optional authentic GBA BIOS usage for maximum compatibility
  - HLE (High-Level Emulation) BIOS implementation included for games that don't require authentic BIOS
  - Configurable per-game or globally
- **Patch Support**: Automatic game patching with IPS, UPS, and PPF formats
- **Sensor Support**: Emulation of motion sensors for compatible games
  - Accelerometer support
  - Gyroscope support
  - Light sensor support (with configurable sensitivity)
  - Automatic sensor detection for known games

### Audio Features

- **PCM audio channels**: 2 channels with independent volume control
- **Game Boy APU**: 4 channels (Pulse #1, Pulse #2, Wave, Noise) with separate volume control
- **Per-channel enable/disable support**
- **Sound filtering and interpolation options**
- **Configurable sample rate mixing**

### Display Features

- RGB565 and 32-bit color output support
- Hardware-accelerated rendering
- Game-specific settings database for automatic configuration

## Building

### Prerequisites

- **C++ Compiler**: GCC 10+, Clang 11+, or MSVC with C++20 support
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

cd GBA.emu
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

Key compiler definitions:
- `HAVE_ZLIB_H` - Enable zlib compression
- `FINAL_VERSION` - Production build flag
- `C_CORE` - Use C core implementation
- `NO_PNG` - Disable PNG support
- `NO_LINK` - Disable link cable emulation
- `NO_DEBUGGER` - Disable debugger features

## BIOS Requirements

**No BIOS files required.** GBA.emu includes HLE (High-Level Emulation) BIOS functionality, so an authentic BIOS is **optional** for most games.

### Optional BIOS Configuration

For enhanced compatibility, you may optionally use an authentic GBA BIOS:

- **File Size**: Exactly 16,384 bytes (16KB / 0x4000 bytes)
- **File Format**: Raw binary dump of GBA BIOS
- **Common Names**: `gba_bios.bin`, `bios.bin`

Configure the BIOS path through the emulator's settings menu. The emulator can operate in three modes:
- **Auto**: Uses BIOS automatically when path is configured
- **Off**: Always uses HLE BIOS
- **On**: Forces authentic BIOS usage (requires valid BIOS file)

Most commercial GBA games work perfectly without an authentic BIOS using the built-in HLE.

## Resources

### Upstream Projects

- **VBA-M (VisualBoyAdvance-M)**
  - Website: https://vba-m.com/
  - GitHub: https://github.com/visualboyadvance-m/visualboyadvance-m
  - License: GPL-3.0
  - Version: Based on VBA-M GIT commit c435107 (December 31, 2024)

- **Gb_Apu**
  - Copyright: Shay Green (blargg)
  - License: LGPL

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

GBA.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **GBA.emu Port**: Copyright © 2012-2025 Robert Broglia
- **VBA-M Components**: Copyright © VBA-M development team
- **Gb_Apu**: Copyright © Shay Green (blargg)
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
