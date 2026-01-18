# NGP.emu

Neo Geo Pocket and Neo Geo Pocket Color emulator for multiple platforms, part of the EX Emulators suite.

## Overview

NGP.emu is a Neo Geo Pocket and Neo Geo Pocket Color emulator based on components from the Mednafen project, specifically the NeoPop emulator core originally developed by neopop_uk.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Neo Geo Pocket (NGP)** - SNK's monochrome handheld console (1998)
- **Neo Geo Pocket Color (NGPC)** - SNK's color handheld console (1999)
  - Display Resolution: 160×152 pixels
  - Aspect Ratio: 20:19 (Original)
  - Frame Rate: ~59.95 Hz
  - Main CPU: Toshiba TLCS-900h (16-bit RISC processor)
  - Sound CPU: Z80 (8-bit)
  - Audio Chip: T6W28 APU (Audio Processing Unit)

### Supported File Formats

- `.ngp` - Neo Geo Pocket ROM
- `.ngc` - Neo Geo Pocket Color ROM
- `.npc` - Alternative NGPC ROM format

## Features

### Emulation Features

- **Full CPU Emulation**: Complete TLCS-900h CPU emulation with instruction interpretation, Z80 sound CPU for audio processing
- **Graphics**: Dual rendering modes (monochrome and color), automatic color mode detection based on ROM, scanline-based rendering for accuracy
- **Audio**: T6W28 APU emulation, configurable audio mixing rates, high-quality sound synthesis
- **Memory & Storage**: Flash memory emulation for game saves, save state support (.mca format), backup memory persistence (.ngf format), 16KB Extended RAM emulation
- **BIOS Handling**: Built-in High-Level Emulation (HLE) BIOS, no external BIOS file required

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

cd NGP.emu
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

**No BIOS files required.** NGP.emu includes a built-in High-Level Emulation (HLE) BIOS that provides all necessary system functionality. The emulator uses a reverse-engineered BIOS implementation with system call interception.

## Resources

### Upstream Projects

- **Mednafen**
  - Website: https://mednafen.github.io/
  - NGP core based on NeoPop emulator
  - License: GPL-2.0

- **Original NeoPop**
  - Original emulator by neopop_uk (2001-2002)
  - Foundation for Mednafen's NGP support

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

NGP.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **NGP.emu Port**: Copyright © 2011-2025 Robert Broglia
- **NeoPop Core**: Copyright © 2001-2002 neopop_uk
- **Mednafen Integration**: Copyright © Mednafen Team
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
