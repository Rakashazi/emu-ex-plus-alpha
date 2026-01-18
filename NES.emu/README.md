# NES.emu

NES/Famicom emulator for multiple platforms, part of the EX Emulators suite.

## Overview

NES.emu is a high-accuracy NES/Famicom emulator based on the FCEUX core, providing comprehensive support for Nintendo Entertainment System and Famicom games across multiple platforms.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Nintendo Entertainment System (NES)** - NTSC (60.099 Hz) and PAL (50 Hz) variants
- **Famicom** - Japanese variant with native support
- **Dendy** - Russian NES clone with custom timings
- **Famicom Disk System (FDS)** - Requires BIOS (see BIOS section below)
- **NES Sound Format (NSF)** - Music/audio demo playback

### Supported File Formats

- `.nes` - iNES ROM format (most common)
- `.unf`, `.unif` - UNIF format (homebrew games)
- `.fds` - Famicom Disk System disk images
- `.nsf` - NES Sound Format (music/demos)

## Features

### Emulation Features

- **Cycle-accurate 6502 CPU emulation** with precise timing
- **Scanline-accurate PPU** (Picture Processing Unit) with new PPU core support
- **Full 2A03 APU emulation** with high-quality FIR filtering
- **184+ cartridge mappers** including MMC1-5, VRC1-7, Namco, Sunsoft, Bandai, and unlicensed mappers
- **Game Genie and PAR cheat support**: 6-digit and 8-digit Game Genie codes, raw hex address cheats
- **Save states** with compression and automatic backup
- **Movie recording/playback** for TAS (Tool-Assisted Speedrun)

### Input Device Support

- Standard NES gamepad (8 buttons)
- 4-Player adapter (Four Score)
- Light Guns (Zapper, LCD Comp Zapper)
- Power Pad (Family Trainer)
- NES Mouse, SNES Mouse
- Arkanoid paddle controller
- Family Keyboard, Subor Keyboard, PEC-586 Keyboard
- Microphone (P2 Start button emulation)

### Video Options

- **Multiple palette options**: Classic, Digital Prime, Lightful, Magnum, Pallightful, Smooth V2, Wavebeam, Custom palette loading (.pal files)
- **Configurable visible scanlines**: 224-240 lines, adjustable start line 0-8
- **Horizontal video crop** support
- **Aspect ratio correction**: 4:3 original or custom
- **Sprite limit disable** option
- **Deinterlacing** support

### Audio Features

- **Sound quality levels**: Low, High, High-Extra
- **Low-pass audio filter** option
- **Duty cycle swapping** for pulse waves
- **Multi-channel support**: 2x Pulse, Triangle, Noise, DPCM, Expansion audio chips

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

cd NES.emu
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
- `HAVE_ASPRINTF` - Use asprintf function
- `PSS_STYLE=1` - Path separator style
- `LSB_FIRST` - Little-endian architecture
- `FRAMESKIP` - Enable frameskip support

## BIOS Requirements

### Famicom Disk System

**Required for FDS (.fds) games:**

- **File**: `disksys.rom` (also accepts `disksys.bin`)
- **Size**: 8 KB (8192 bytes)

**Setup:**
1. Navigate to "System Options" → "Disk System BIOS"
2. Select your BIOS file
3. The path will be saved in the configuration

**Note:** The emulator will display an error message "No FDS BIOS set" or "Incompatible FDS BIOS" if the file is missing or invalid when attempting to load FDS games.

## Resources

### Upstream Projects

- **FCEUX**
  - Website: http://fceux.com
  - GitHub: https://github.com/TASEmulators/fceux
  - Copyright: FCEUX Development Team
  - License: GPL-2.0 or later

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

NES.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **NES.emu Port**: Copyright © 2011-2025 Robert Broglia
- **FCEUX Core**: Copyright © FCEUX Development Team (zeromus, adelikat, AnS, CaH4e3, and others)
- **Original FCEU**: Copyright © 2002-2003 Xodnizel
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
