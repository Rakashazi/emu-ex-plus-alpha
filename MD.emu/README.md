# MD.emu

Sega Genesis/Mega Drive emulator for multiple platforms, part of the EX Emulators suite.

## Overview

MD.emu is a cross-platform Sega Genesis/Mega Drive emulator based on the Genesis Plus GX emulator core. It provides accurate emulation of Sega's 16-bit console and related systems with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Sega Genesis / Mega Drive** - The primary 16-bit game console
- **Sega CD / Mega-CD** - CD-ROM add-on for the Genesis/Mega Drive
- **Sega Master System** (SMS) - 8-bit predecessor to the Genesis

### Supported File Formats

**Cartridge ROMs:**
- `.bin` - Binary ROM dump
- `.smd` - SMD format ROM
- `.md` - Mega Drive ROM
- `.gen` - Genesis ROM
- `.sms` - Master System ROM

**CD-ROM Games:**
- `.cue` - CUE sheet (with associated BIN/ISO)
- `.iso` - ISO disc image
- `.chd` - Compressed Hunks of Data format

## Features

### Emulation Features

- **Accurate Genesis Plus GX Core**: Based on the well-regarded Genesis Plus emulation core by Charles Mac Donald and Eke-Eke
- **Sega CD Support**: Full Sega CD/Mega-CD emulation with CD audio support (CDDA)
- **Multiple Region Support**: Auto-detection or manual selection of USA, Europe, Japan (NTSC), and Japan (PAL) regions
- **Video System Selection**: Auto-detection or manual PAL/NTSC selection
- **Cycle-Accurate Emulation**: Accurate timing for both 68000 and Z80 processors

### CPU Emulation

- **Motorola 68000**: Main CPU using Musashi emulator (v3.3) by Karl Stenerud
- **Zilog Z80**: Sound CPU with custom implementation
- **Sub-CPU (Sega CD)**: Additional 68000 for CD operations

### Sound Emulation

- **YM2612 FM Synthesis**: Genesis/Mega Drive FM sound chip
- **YM2413 (FM)**: Optional Master System FM sound unit support
- **SN76489 PSG**: Programmable Sound Generator
- **PCM Audio**: Sega CD PCM chip
- **CD Audio (CDDA)**: Full CD audio track playback
- **High-Quality Audio**: Configurable audio quality and filtering with Fir_Resampler

### Special Hardware Support

- **SVP Chip**: Samsung SSP1601 DSP emulation for Virtua Racing
- **EEPROM**: Save game support for games using EEPROM storage
- **SRAM**: Battery-backed RAM with configurable endianness
- **Action Replay / Game Genie**: Cheat device support
- **BRAM**: Sega CD internal backup RAM

### Input Devices

- **3-Button Gamepad**: Standard Genesis controller
- **6-Button Gamepad**: Extended controller with X/Y/Z buttons and Mode
- **4-Player Adapter**: Team Player multitap support
- **Light Guns**: Menacer and Justifier support
- **Mouse**: Sega Mouse support
- **Other**: Activator, Sports Pad, Paddle, XE-A1P analog controller

### Video Features

- **Multiple Render Formats**: Support for 16-bit (RGB565) and 32-bit (RGBA8888) rendering
- **NTSC Video Filter**: Optional NTSC composite/S-Video/RGB filtering
- **PAL Video System**: Full PAL timing support
- **Dynamic Resolution**: Automatic viewport adjustment
- **Aspect Ratio**: 4:3 original aspect ratio support

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

cd MD.emu
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

Key build defines:
- `LSB_FIRST` - Little-endian architecture
- `NO_SYSTEM_PICO` - Disables Pico emulation
- `MDFN_CD_NO_CCD` - Disables CCD disc format support

## BIOS Requirements

### Sega CD BIOS

To play Sega CD games, you **must** provide the appropriate BIOS file for your region:

| Region | BIOS File Needed | Configure In |
|--------|-----------------|--------------|
| USA | USA Sega CD BIOS | Options → CD BIOS → USA CD BIOS |
| Europe | European Sega CD BIOS | Options → CD BIOS → Europe CD BIOS |
| Japan | Japanese Sega CD BIOS | Options → CD BIOS → Japan CD BIOS |

### BIOS Setup

1. Obtain the correct BIOS file for your region (typically 128KB .bin file)
2. Launch MD.emu and go to Options
3. Navigate to the BIOS section
4. Set the path for your region's BIOS
5. The emulator will validate the BIOS when loading a CD game

### Region Selection

The emulator will:
- Auto-detect the region from the CD boot sector (default)
- Use the region specified in Options → Game Region
- Load the corresponding BIOS file

**Note:** The BIOS must be a valid Sega CD BIOS ROM. The emulator checks for the BIOS marker to verify validity.

## Resources

### Upstream Projects

- **Genesis Plus GX**
  - Website: https://segaretro.org/Genesis_Plus
  - Authors: Charles Mac Donald, Eke-Eke (2007-2011)
  - License: GPL-2.0 or later

- **Musashi 68000 Emulator**
  - Version: 3.3
  - Author: Karl Stenerud
  - Website: http://kstenerud.cjb.net
  - License: MIT-style

- **Mednafen CD Support**
  - CD access and CHD format support
  - Used for Sega CD emulation
  - License: GPL-2.0

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

MD.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **MD.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Genesis Plus GX**: Copyright © Charles Mac Donald, Eke-Eke
- **Musashi 68000 Emulator**: Copyright © Karl Stenerud
- **Mednafen CD Support**: Copyright © Mednafen Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
