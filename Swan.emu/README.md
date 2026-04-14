# Swan.emu

WonderSwan and WonderSwan Color emulator for multiple platforms, part of the EX Emulators suite.

## Overview

Swan.emu is a cross-platform emulator for the Bandai WonderSwan and WonderSwan Color handheld game consoles. It uses components from the Mednafen project (specifically, the Cygne WonderSwan emulation core) to provide accurate emulation with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **WonderSwan** - Original monochrome handheld (1999)
- **WonderSwan Color** - Color version with backward compatibility (2000)
- **SwanCrystal** - Final revision with improved screen (2002)

### System Specifications

- **CPU**: NEC V30 MZ @ 3.072 MHz (fast version of V30 with 16-byte prefetch pipeline)
- **Display**: 224×144 pixels (28×18 tiles)
- **Colors**: WonderSwan: 16 shades of gray (8 simultaneously, 16 palettes); WonderSwan Color: 4096 colors (241 simultaneously from 12-bit RGB palette)
- **RAM**: 16 KB (WS) / 64 KB (WSC)
- **Audio**: 4 PCM channels with 4-bit samples @ 24 KHz
- **ROM Support**: Up to 64 MB

### Supported File Formats

- `.ws` - WonderSwan ROM images
- `.wsc` - WonderSwan Color ROM images
- `.bin` - Generic binary ROM format
- `.wsr` - WonderSwan Music Rip files (special audio playback format)

## Features

### WonderSwan-Specific Features

- **User Profile Configuration**: Configurable user name, birth date, gender, blood type, language preference (some games use this data for personalization)
- **Rotation Support**: Auto-rotation based on ROM header, manual horizontal/vertical orientation override, virtual gamepad adapts to rotation setting
- **Real-Time Clock (RTC) Support**: Emulates cartridge RTC for compatible games, saves RTC state across sessions
- **Save Data Support**: SRAM (8 KB to 512 KB), EEPROM (128 bytes, 1 KB, or 2 KB), automatic save file management
- **Virtual Gamepad Customization**: Configurable button visibility based on orientation

### Input System

The WonderSwan features a unique control scheme with:
- **X-axis buttons**: X1 (Up), X2 (Right), X3 (Down), X4 (Left)
- **Y-axis buttons**: Y1, Y2, Y3, Y4
- **Action buttons**: A, B
- **System button**: Start

The emulator handles automatic input rotation for games designed for vertical orientation.

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

cd Swan.emu
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

**No BIOS files required.** The WonderSwan and WonderSwan Color did not have system BIOS - games contained all necessary code in their ROM chips.

## Resources

### Upstream Projects

- **Mednafen**
  - Website: https://mednafen.github.io/
  - WonderSwan emulation core based on Cygne
  - License: GPL-2.0
  - Original core by Dox (dox@space.pl)

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

Swan.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **Swan.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Mednafen WonderSwan Core**: Copyright © 2002 Dox, 2007-2017 Mednafen Team
- **Original Cygne Emulator**: Dox (dox@space.pl)
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
