# NEO.emu

Neo Geo arcade system emulator for multiple platforms, part of the EX Emulators suite.

## Overview

NEO.emu is a Neo Geo arcade system and home console (AES) emulator based on the GnGeo emulator. It provides accurate emulation of SNK's Neo Geo hardware with support for hundreds of games.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Neo Geo MVS (Multi Video System)** - Arcade cabinet system
- **Neo Geo AES (Advanced Entertainment System)** - Home console version

### Supported File Formats

- `.zip` - Standard ZIP archives (most common)
- `.7z` - 7-Zip archives
- `.rar` - RAR archives

## Features

### Emulation Features

- **High Compatibility**: Supports hundreds of Neo Geo game ROMs
- **Multiple CPU Cores**:
  - Cyclone (ARM assembly, optimized for ARM devices)
  - Musashi (Portable C/C++ implementation for x86 and other architectures)
- **Z80 Audio CPU**: Full audio emulation with YM2610 sound chip
- **Save States**: Full save state support (.sta format)
- **SRAM/Memory Card**: Automatic save of NVRAM (.nv) and memory card data (.memcard)
- **ROM Cache**: Optional .gno cache file generation for faster loading
- **Strict ROM Checking**: Optional CRC verification for ROM accuracy

### Special Features

- **Unibios Integration**: Region switching and system mode switching
- **Game List Browser**: Built-in game database with 200+ supported titles
- **ROM Decryption**: Automatic decryption for encrypted ROMs
- **Timer Interrupt Emulation**: Configurable for game compatibility

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

cd NEO.emu
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
- `HAVE_CONFIG_H` - Enable configuration header
- `LSB_FIRST` - Little-endian byte order

## BIOS Requirements

NEO.emu requires Neo Geo BIOS files to function. These must be placed in the same directory as your ROM files.

### Required BIOS Files

The emulator needs the following BIOS ROM files in a `neogeo.zip` archive:

**For Unibios (Recommended):**
- `uni-bios_2_3.rom` - Unibios 2.3 (default)
- `uni-bios_3_0.rom` - Unibios 3.0
- `uni-bios_3_1.rom` - Unibios 3.1
- `uni-bios_3_2.rom` - Unibios 3.2
- `uni-bios_3_3.rom` - Unibios 3.3
- `uni-bios_4_0.rom` - Unibios 4.0

**For MVS/AES BIOS:**
- `neo-geo.rom` or `sp-s2.sp1` - Main BIOS
- `000-lo.lo` - Zoom lookup table
- `sfix.sfix` or `sfix.sfx` - Fix layer graphics
- `sm1.sm1` - Audio BIOS (Z80)

### BIOS File Placement

Place the `neogeo.zip` file containing the BIOS ROMs in the same directory as your game ROM files or the emulator's content search path.

The emulator also requires `gngeo_data.zip` which contains ROM definitions (.drv files for each game). This file is included with the emulator in the `res/` directory.

## Resources

### Upstream Projects

- **GnGeo**
  - Website: https://code.google.com/p/gngeo
  - Copyright: Peponas Mathieu (2001)
  - License: GPL-2.0 or later

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

NEO.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **NEO.emu Port**: Copyright © 2012-2025 Robert Broglia
- **GnGeo Core**: Copyright © 2001 Peponas Mathieu and GnGeo Team
- **MAME Components**: Copyright © MAME Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
