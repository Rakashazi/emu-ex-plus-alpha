# GBC.emu

Game Boy and Game Boy Color emulator for multiple platforms, part of the EX Emulators suite.

## Overview

GBC.emu is a cross-platform Game Boy and Game Boy Color emulator based on the Gambatte emulator core. It provides highly accurate, cycle-accurate emulation of Nintendo's classic handheld consoles with modern features and multi-platform support.

**Author:** Robert Broglia
**Website:** https://www.explusalpha.com/
**License:** GNU General Public License v3.0

## Supported Systems

- **Game Boy (GB/DMG)** - Original monochrome Game Boy (1989)
- **Game Boy Color (GBC)** - Enhanced color handheld console (1998)

### Supported File Formats

- `.gb` - Game Boy ROM files
- `.gbc` - Game Boy Color ROM files
- `.dmg` - Game Boy ROM files (alternative extension)

## Features

### Emulation Features

- **Cycle-Accurate Emulation**: Based on Gambatte, providing highly accurate Game Boy/Game Boy Color emulation
- **Real-Time Clock (RTC) Support**: Full RTC emulation for games that use it (e.g., Pokémon Gold/Silver/Crystal)
- **Battery-Backed Save RAM**: Automatic save/load of battery-backed SRAM to `.sav` files
- **RTC State Persistence**: RTC state saved to `.rtc` files
- **Save State Support**: `.gqs` files (Gambatte QuickSave format) with 10 save state slots

### Game Boy Palette System

For Game Boy (non-color) games, GBC.emu includes multiple color palettes:

- **13 Built-in Palettes**: Original, Brown, Red, Dark Brown, Pastel, Orange, Yellow, Blue, Dark Blue, Gray, Green, Dark Green, Reverse
- **Game-Specific Built-in Palettes**: Automatic detection and application for specific Game Boy titles
- **Color Conversion Options**: Saturated GBC Colors and multiple color conversion modes

### Audio Features

- **Multiple Resampler Options**: Up to 4 different audio resampling algorithms available
  - Kaiser50sinc
  - Kaiser70sinc
  - And additional resampling methods
- **High-Quality Audio Resampling**: Specialized resample chain with configurable quality

### Special Features

- **Report Hardware as GBA**: Option to report the system as Game Boy Advance for enhanced game compatibility
- **Cheat Support**:
  - Game Genie Codes (format `HHH-HHH-HHH`)
  - Game Shark Codes (format `01HHHHHH`)
  - Automatic save/load of cheats to `.gbcht` files
- **Memory Bank Controllers (MBC)**: Full support for all standard Game Boy MBCs (MBC0, MBC1, MBC2, MBC3, MBC5, and specialized controllers)

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

cd GBC.emu
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
- `HAVE_STDINT_H` - Standard integer types support
- `GAMBATTE_NO_OSD` - Disables Gambatte's built-in OSD

## BIOS Requirements

**No BIOS files required.** GBC.emu includes built-in initialization routines and does not require Game Boy or Game Boy Color BIOS/boot ROM files to operate.

## Resources

### Upstream Projects

- **Gambatte**
  - Website: http://gambatte.sourceforge.net/
  - GitHub: https://sourceforge.net/projects/gambatte/
  - License: GPL-2.0
  - Author: sinamas

- **Imagine Framework**
  - GitHub: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: http://www.explusalpha.com/home/imagine
  - License: GPL-3.0

- **EX Emulators Suite**
  - Main Project: https://github.com/Rakashazi/emu-ex-plus-alpha
  - Website: https://www.explusalpha.com/

## License

GBC.emu is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See `COPYING.GPL` for the full license text.

## Credits

- **GBC.emu Port**: Copyright © 2011-2025 Robert Broglia
- **Gambatte Core**: Copyright © 2007 sinamas
- **Imagine Framework**: Copyright © 2010-2025 Robert Broglia
