# C64.emu

Commodore 8-bit computer emulator for multiple platforms, part of the EX Emulators suite.

## Overview

C64.emu is a cross-platform Commodore 8-bit computer emulator based on the VICE emulator core (version 3.10). It provides accurate emulation of the Commodore 64 and other Commodore 8-bit systems with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **C64** - Standard Commodore 64 (fast emulation)
- **C64 (Cycle Accurate)** - Commodore 64 with cycle-accurate VICII emulation
- **C64DTV** - Commodore 64 Direct-to-TV variants
- **C128** - Commodore 128 (with C64 compatibility mode)
- **C64 SuperCPU** - C64 with SuperCPU accelerator
- **VIC-20** - Commodore VIC-20
- **CBM-II 6x0** - CBM-II 600/700 series business computers
- **CBM-II 5x0** - CBM-II 500 series
- **PET** - Commodore PET series
- **Plus/4** - Commodore Plus/4

### Model Variants

Each system supports multiple hardware model variants (PAL/NTSC, different revisions):

**C64 Models**: C64 PAL/NTSC, C64C PAL/NTSC, Old C64, Drean, C64 SX, Japanese, C64 GS, PET64, MAX Machine

**C64DTV Models**: DTV v2/v3 PAL/NTSC, Hummer NTSC

**C128 Models**: C128/C128D/C128DCR in PAL and NTSC variants

**VIC-20 Models**: VIC-20 PAL/NTSC variants

**PET Models**: PET 2001-8N, 3008, 3016, 3032, 3032B, 4016, 4032, 4032B, 8032, 8096, 8296, SuperPET

**Plus/4 Models**: Plus/4 PAL/NTSC, C16 PAL/NTSC, C116 PAL, V364 NTSC

**CBM-II Models**: CBM 510/610/620/710/720 in various configurations

### Supported File Formats

- `.d64`, `.d67`, `.d71`, `.d80`, `.d81`, `.d82`, `.d1m`, `.d2m`, `.d4m` - Disk images
- `.g64`, `.p64`, `.g41`, `.x64`, `.dsk` - Advanced disk formats
- `.t64`, `.tap` - Tape images
- `.crt`, `.bin` - Cartridge images
- `.20`, `.40`, `.60`, `.70`, `.a0`, `.b0` - VIC-20 headerless cartridges
- `.prg`, `.p00` - Program files

## Features

### Emulation Features

- **Multiple SID Engine Support**:
  - FastSID (fast CPU-friendly emulation)
  - ReSID (high-quality cycle-accurate emulation)
  - ReSID-DTV (for C64DTV models)
  - Multiple ReSID sampling quality modes (Fast, Interpolation, Resampling, Fast Resampling)

- **Drive Emulation**:
  - Support for up to 4 disk drives (units 8-11)
  - True Drive Emulation mode for accurate 1541/1571/1581 emulation
  - Virtual Device Traps for faster loading
  - Multiple drive types: 1540, 1541, 1541-II, 1551, 1570, 1571, 1581, 2000, 4000, 2031, 2040, 3040, 4040, 1001, 8050, 8250

- **Cartridge System**:
  - Extensive cartridge support including Action Replay, Final Cartridge, EasyFlash, Retro Replay, and many others
  - Over 100 different cartridge types supported
  - CRT format with automatic type detection

- **Video Options**:
  - Configurable border modes (Normal, Full, Debug)
  - Border cropping options
  - External palette file support (.vpl format)
  - Color adjustment controls (Saturation, Contrast, Brightness, Gamma, Tint)
  - PAL and NTSC video standards
  - 4:3 aspect ratio (original) support

- **Advanced Features**:
  - Autostart support with optional warp mode
  - State save/load support
  - Real-Time Clock (RTC) emulation
  - RAM expansion modules (REU, GEO-RAM, etc.)
  - VIC-20 RAM expansion blocks
  - Printer emulation
  - RS232 device emulation (configurable)

## Building

### Prerequisites

- **C++ Compiler**: GCC 16+, Clang 21+, or MSVC with C++23 support
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

cd C64.emu
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

The build system creates multiple shared libraries for each emulated system:
- `libc64.so` - Standard C64
- `libc64sc.so` - Cycle-accurate C64
- `libc64dtv.so` - C64DTV
- `libc128.so` - C128
- `libscpu64.so` - SuperCPU
- `libvic.so` - VIC-20
- `libpet.so` - PET
- `libplus4.so` - Plus/4
- `libcbm2.so` - CBM-II 6x0
- `libcbm5x0.so` - CBM-II 5x0

## BIOS Requirements

C64.emu requires VICE system files (ROMs and other data files) to function. These files are **not included** with the emulator and must be obtained separately.

### System File Locations

**Linux:**
- `~/.local/share/C64.emu/`
- `/usr/share/games/vice/`

**Other platforms:**
- Configure via: Options → File Paths → VICE System Files

### Required System Files

Each emulated system requires its own set of ROM files:

**C64/C64SC:**
- `basic` - BASIC ROM
- `chargen` - Character generator ROM
- `kernal` - KERNAL ROM
- `d1541II` - 1541-II drive ROM
- Various other drive ROMs

**C128:**
- `basiclo`, `basichi` - BASIC ROMs (low/high)
- `chargde`, `chargen` - Character ROMs
- `kernal`, `kernal64` - KERNAL ROMs
- VDC ROM

**VIC-20:**
- `basic` - BASIC ROM
- `chargen` - Character ROM
- `kernal` - KERNAL ROM

**Plus/4:**
- `basic`, `kernal` - System ROMs
- `3plus1lo`, `3plus1hi` - Built-in software ROMs

### Obtaining System Files

System files can be obtained from:
1. Original VICE emulator distribution (https://vice-emu.sourceforge.io/)
2. Extracted from original hardware ROMs (if you own the hardware)
3. Legal ROM archives

**Note:** The first time you run C64.emu without proper system files, you will receive an error message indicating which file is missing.

## Resources

### Upstream Projects

- **VICE Emulator**
  - Website: https://vice-emu.sourceforge.io/
  - GitHub: https://sourceforge.net/projects/vice-emu/
  - License: GPL-2.0 or later
  - Version: 3.10

- **ReSID Engine**
  - Copyright: Dag Lem and the ReSID team
  - License: GPL

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

C64.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **C64.emu Port**: Copyright © 2013-2025 Robert Broglia
- **VICE Emulator Core**: Copyright © The VICE Team
- **ReSID Engine**: Copyright © Dag Lem and the ReSID team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
