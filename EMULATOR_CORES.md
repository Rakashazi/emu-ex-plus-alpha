# Emulator Cores Reference

This document provides a comprehensive overview of the upstream emulator cores used by each emulator in the emu-ex-plus-alpha project.

## Overview

The emu-ex-plus-alpha project is a unified frontend framework that integrates multiple well-established emulator cores, providing them with:
- Consistent cross-platform support (Linux, Android, iOS, Pandora, etc.)
- Unified UI and configuration system via the **Imagine Framework**
- Modern features (save states, input remapping, etc.) via **EmuFramework**
- Mobile-optimized controls and interfaces

**Project Maintainer:** Robert Broglia
**Website:** https://www.explusalpha.com/
**Repository:** https://github.com/Rakashazi/emu-ex-plus-alpha
**License:** GNU General Public License v3.0

---

## Emulator Core Details

### 1. 2600.emu - Atari 2600 Emulator

**Upstream Core:** Stella
**Official Website:** https://stella-emu.github.io/
**Source Code:** https://github.com/stella-emu/stella
**Documentation:** https://stella-emu.github.io/docs/
**License:** GPL-3.0
**Description:** The Stella emulator core provides accurate Atari 2600 emulation with support for 54+ cartridge mappers, multiple controller types, and advanced video/audio features.

---

### 2. C64.emu - Commodore 64 Emulator

**Upstream Core:** VICE (Versatile Commodore Emulator) version 3.10
**Official Website:** https://vice-emu.sourceforge.io/
**Source Code:** https://sourceforge.net/projects/vice-emu/
**Documentation:** https://vice-emu.sourceforge.io/vice_toc.html
**License:** GPL-2.0 or later
**Systems Emulated:** C64, C64SC (cycle-accurate), C64DTV, C128, SuperCPU, VIC-20, CBM-II, PET, Plus/4
**Description:** VICE provides comprehensive emulation of Commodore 8-bit computers with accurate CPU, VIC/VIC-II graphics, SID sound (FastSID/ReSID), and extensive drive emulation.

**Additional Components:**
- **ReSID**: High-quality SID chip emulation by Dag Lem

---

### 3. GBA.emu - Game Boy Advance Emulator

**Upstream Core:** VBA-M (VisualBoyAdvance-M)
**Official Website:** https://vba-m.com/
**Source Code:** https://github.com/visualboyadvance-m/visualboyadvance-m
**Version Used:** Based on VBA-M GIT commit c435107 (December 31, 2024)
**License:** GPL-2.0 or later
**Description:** VBA-M provides mature and accurate GBA emulation with multiple save types, RTC emulation, sensor support, and advanced audio features.

**Additional Components:**
- **Gb_Apu**: Game Boy audio processing unit by Shay Green (blargg)

---

### 4. GBC.emu - Game Boy / Game Boy Color Emulator

**Upstream Core:** Gambatte
**Official Website:** http://gambatte.sourceforge.net/
**Source Code:** https://sourceforge.net/projects/gambatte/
**License:** GPL-2.0
**Author:** sinamas
**Description:** Gambatte provides highly accurate, cycle-accurate Game Boy and Game Boy Color emulation with precise timing and compatibility.

---

### 5. Lynx.emu - Atari Lynx Emulator

**Upstream Core:** Handy (via Mednafen)
**Mednafen Website:** https://mednafen.github.io/
**Original Handy Author:** K. Wilkins (1996-1997, 2004)
**License:** zlib-style permissive license (Handy), GPL-2.0 (Mednafen)
**Description:** Handy emulator core integrated into Mednafen, providing accurate Atari Lynx emulation with display rotation support and optional BIOS.

---

### 6. MD.emu - Sega Genesis/Mega Drive Emulator

**Upstream Core:** Genesis Plus GX
**Official Website:** https://segaretro.org/Genesis_Plus
**Source Code:** Integrated into project (originally from various sources)
**Original Author:** Charles Mac Donald
**GX Port Author:** Eke-Eke (2007-2011)
**License:** GPL-2.0 or later
**Systems Emulated:** Sega Genesis/Mega Drive, Sega CD/Mega-CD, Sega Master System
**Description:** Genesis Plus provides accurate emulation of Sega's 16-bit hardware with full Sega CD support, SVP chip emulation, and extensive peripheral support.

**Additional Components:**
- **Musashi 68000 Emulator** v3.3: Portable Motorola 68000 emulator by Karl Stenerud (http://kstenerud.cjb.net)
- **Mednafen CD Support**: CD-ROM access and CHD format support

---

### 7. MSX.emu - MSX Computer Emulator

**Upstream Core:** BlueMSX
**Official Website:** http://www.bluemsx.com
**Source Code:** Integrated into project
**Primary Author:** Daniel Vik
**License:** GPL-2.0 or later
**Systems Emulated:** MSX1, MSX2, MSX2+, MSX Turbo-R, ColecoVision, SG-1000, SVI-328, Coleco Adam
**Description:** BlueMSX is one of the most accurate and feature-complete MSX emulators, with extensive hardware emulation including Z80/R800 CPU, multiple video chips, and comprehensive sound chip support.

**Bundled BIOS:**
- **C-BIOS** v0.29a (2018-09-23): Open-source BIOS replacement (http://cbios.sourceforge.net/)
- **License:** BSD-style
- **Authors:** BouKiCHi, Maarten ter Huurne, Albert Beevendorp, and contributors

---

### 8. NEO.emu - Neo Geo Emulator

**Upstream Core:** GnGeo
**Official Website:** https://code.google.com/p/gngeo (archived)
**Author:** Peponas Mathieu (2001)
**License:** GPL-2.0 or later
**Systems Emulated:** Neo Geo MVS (arcade), Neo Geo AES (home console)
**Description:** GnGeo provides Neo Geo emulation with support for hundreds of games, Unibios integration, and automatic ROM decryption.

**Additional Components:**
- **MAME YM2610**: Sound chip emulation from MAME project

---

### 9. NES.emu - NES/Famicom Emulator

**Upstream Core:** FCEUX (FCE Ultra X)
**Official Website:** http://fceux.com
**Source Code:** https://github.com/TASEmulators/fceux
**Documentation:** FCEUX wiki and user manual
**License:** GPL-2.0 or later
**Systems Emulated:** NES (NTSC/PAL), Famicom, Dendy, Famicom Disk System, NSF (music format)
**Description:** FCEUX provides high-accuracy NES/Famicom emulation with cycle-accurate CPU, scanline-accurate PPU, 184+ cartridge mappers, and extensive peripheral support.

**Development Team:**
- FCEUX Team: zeromus, adelikat, AnS, CaH4e3, xhainingx, gocha, mjbudd77, Plombo, qeed, QFox, Shinydoofy, UncombedCoconut, and others
- Original FCEU: Xodnizel (2002-2003)

---

### 10. NGP.emu - Neo Geo Pocket Emulator

**Upstream Core:** NeoPop (via Mednafen)
**Mednafen Website:** https://mednafen.github.io/
**Original NeoPop Author:** neopop_uk (2001-2002)
**License:** GPL-2.0
**Systems Emulated:** Neo Geo Pocket, Neo Geo Pocket Color
**Description:** NeoPop core maintained by Mednafen project, providing accurate NGP/NGPC emulation with built-in HLE BIOS.

---

### 11. PCE.emu - TurboGrafx-16/PC Engine Emulator

**Upstream Core:** Mednafen (pce and pce_fast modules)
**Official Website:** https://mednafen.github.io/
**Documentation:** https://mednafen.github.io/documentation/
**License:** GPL-2.0
**Systems Emulated:** PC Engine/TurboGrafx-16, SuperGrafx, TurboGrafx-CD/PC Engine CD-ROM², Arcade Card
**Description:** Mednafen's PCE cores provide accurate PC Engine emulation with dual core options (pce_fast for performance, pce for accuracy), CD-ROM support, and extensive input device emulation.

**Cores:**
- **pce_fast**: Optimized for performance
- **pce**: Higher accuracy mode

---

### 12. Saturn.emu - Sega Saturn Emulator

**Upstream Core:** Mednafen
**Official Website:** https://mednafen.github.io/
**License:** GPL-2.0
**Copyright:** Mednafen Team (2015-2025)
**Systems Emulated:** Sega Saturn (NTSC/PAL), ST-V (Sega Titan Video arcade)
**Description:** Mednafen's Saturn core provides accurate emulation of Sega Saturn hardware including dual SH-2 CPUs, VDP1/VDP2 graphics processors, SCU with DSP, SCSP sound, and extensive peripheral support.

**Performance Note:** Requires 64-bit device for optimal performance

---

### 13. Swan.emu - WonderSwan Emulator

**Upstream Core:** Cygne (via Mednafen)
**Mednafen Website:** https://mednafen.github.io/
**Original Cygne Author:** Dox (dox@space.pl, 2002)
**License:** GPL-2.0
**Systems Emulated:** WonderSwan, WonderSwan Color, SwanCrystal
**Description:** Cygne WonderSwan emulation core maintained by Mednafen, providing accurate emulation with unique features like user profile configuration and rotation support.

---

## Framework Components

All emulators are built on top of:

### Imagine Framework
**Description:** Cross-platform application framework providing graphics, input, audio, and windowing support
**Website:** http://www.explusalpha.com/home/imagine
**GitHub:** https://github.com/Rakashazi/emu-ex-plus-alpha
**License:** GPL-3.0
**Author:** Robert Broglia

### EmuFramework
**Description:** Common emulator frontend framework providing shared infrastructure for all EX emulators
**Features:** Save states, configuration management, UI menus, input mapping
**License:** GPL-3.0
**Author:** Robert Broglia

---

## Mednafen-Based Emulators

Multiple emulators use cores from the Mednafen multi-system emulator:

- **Lynx.emu**: Handy core
- **NGP.emu**: NeoPop core
- **PCE.emu**: pce/pce_fast cores
- **Saturn.emu**: Saturn core
- **Swan.emu**: Cygne core

**Mednafen Project:**
Website: https://mednafen.github.io/
License: GPL-2.0
Description: Multi-system emulator with accurate cores for various retro gaming systems

---

## Build Requirements

All emulators require:
- **C++ Compiler**: GCC 16+, Clang 21+, or MSVC with C++20 support
- **Build Tools**: CMake 4.1+, GNU Make, pkg-config
- **Imagine SDK**: Must be built first

Platform support: Linux, Android, iOS, Pandora, and other platforms via Imagine framework

---

## License Summary

- **emu-ex-plus-alpha Framework**: GPL-3.0
- **Most Emulator Cores**: GPL-2.0 or later
- **Some Components**: Various permissive licenses (zlib-style, BSD, MIT)
- See individual emulator README files for detailed license information

---

**Last Updated:** 2025-01-17
**Maintained By:** Robert Broglia
**Project:** emu-ex-plus-alpha
