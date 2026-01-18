# MSX.emu

MSX computer emulator for multiple platforms, part of the EX Emulators suite.

## Overview

MSX.emu is a cross-platform MSX emulator based on components from the BlueMSX emulator core. It provides comprehensive emulation of various MSX computer systems and compatible hardware with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **MSX1** - Original MSX standard with TMS99x8A video chip (16KB VRAM)
- **MSX2** - Enhanced MSX with V9938 video chip (128KB VRAM)
- **MSX2+** - Further enhanced with V9958 video chip and MSX Music (YM2413) support
- **MSX Turbo-R** - Advanced MSX variant with R800 CPU (partial support)
- **ColecoVision** - Coleco's home video game console
- **SG-1000** - Sega's early gaming system
- **SVI-328/328 MK II** - Spectravideo home computers
- **Coleco Adam** - Coleco's home computer variant

### Supported File Formats

- `.rom` - ROM cartridge images
- `.mx1` - MSX1 ROM images
- `.mx2` - MSX2 ROM images
- `.dsk` - Disk images (floppy disk)
- `.cas` - Cassette tape images
- `.col` - ColecoVision ROM images

## Features

### Emulation Features

- **Accurate CPU Emulation**: Full Z80 and R800 (Turbo-R) CPU emulation
- **Video Display Processors**:
  - TMS99x8A (MSX1)
  - V9938 (MSX2)
  - V9958 (MSX2+)
  - Support for all standard screen modes (SCREEN 0-12)
- **Comprehensive Sound Chip Emulation**:
  - PSG (AY-3-8910)
  - SCC/SCC+ (Sound Cartridge Chip)
  - MSX Music (YM2413/OPLL)
  - MSX Audio (Y8950/OPLC)
  - Moonsound (OPL4/YMF278B)
  - Yamaha SFG (FM Sound Generator)
  - PCM/MIDI support
  - VLM5030 voice synthesis
  - SN76489 (ColecoVision/SG-1000)

### Storage and I/O

- **ROM Cartridge Support**: 80+ mapper types including Konami4, Konami5, ASCII8, ASCII16, MegaROM cartridges, and special cartridges
- **Floppy Disk Emulation**: WD2793 FDC with support for .dsk images (2 drives)
- **Cassette Tape**: Read-only support for .cas tape images
- **Hard Disk**: IDE/Sunrise IDE emulation
- **CD-ROM Interface**: Architecture stub support

### Input Devices

- **Full Keyboard Support**: 92-key MSX keyboard mapping
- **Joystick**: MSX joystick port emulation
- **Mouse**: MSX mouse support
- **Special Controllers**: ColecoVision steering wheel, Super Action Controller

### Advanced Features

- **Save States**: Full save/load state support (.sta format)
- **Real-Time Clock**: RTC emulation
- **Printer Interface**: Printer I/O support
- **MIDI I/O**: MIDI input/output device support
- **Audio Mixer**: Per-channel volume and pan control for all sound sources
- **Fast-Forward**: Skip FDC access for faster loading
- **Directory as Disk**: Mount host directories as virtual disks

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

cd MSX.emu
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
- `LSB_FIRST` - Little-endian byte order
- `NO_ASM` - Disables assembly optimizations
- `NO_EMBEDDED_SAMPLES` - Excludes embedded audio samples

## BIOS Requirements

**Bundled C-BIOS included.** MSX.emu includes **C-BIOS 0.29**, an open-source BIOS replacement that allows running MSX cartridge ROMs without requiring proprietary BIOS files.

### C-BIOS Information

- **Version**: 0.29a (2018-09-23)
- **License**: BSD-style (see res/firmware/cbios.txt for full license)
- **Website**: http://cbios.sourceforge.net/
- **Limitations**: Primarily supports cartridge ROM execution; disk BIOS functionality is limited

### Included Machine Configurations

Three pre-configured machine profiles are included in `res/firmware/Machines/`:

1. **MSX - C-BIOS**: Basic MSX1 configuration
2. **MSX2 - C-BIOS**: MSX2 with V9938 video and CMOS support
3. **MSX2+ - C-BIOS**: MSX2+ with V9958 video and MSX Music

### Optional Commercial BIOS

For enhanced compatibility and disk support, you can use commercial MSX BIOS files by placing BIOS ROM files in the firmware directory and creating machine configuration files.

## Resources

### Upstream Projects

- **BlueMSX**
  - Website: http://www.bluemsx.com
  - Author: Daniel Vik
  - Copyright: 2003-2006 Daniel Vik and BlueMSX Team
  - License: GPL-2.0 or later

- **C-BIOS**
  - Website: http://cbios.sourceforge.net/
  - Copyright: 2002-2018 BouKiCHi, Maarten ter Huurne, Albert Beevendorp, and contributors
  - License: BSD-style

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

MSX.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **MSX.emu Port**: Copyright © 2011-2025 Robert Broglia
- **BlueMSX Core**: Copyright © 2003-2006 Daniel Vik and the BlueMSX Team
- **C-BIOS**: Copyright © 2002-2018 BouKiCHi, Maarten ter Huurne, Albert Beevendorp, and contributors
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
