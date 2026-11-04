> Experimental Android bridge proposal for [Rakashazi/emu-ex-plus-alpha](https://github.com/Rakashazi/emu-ex-plus-alpha), based on commit `1c12fac5ce49badaadff2e2f210dcc30b89f4943`. See [proposal, build and security scope](custom/README.md). This is not an official release; no modified emulator APK is distributed. Original credits and licenses remain unchanged.

# EX Emulators

The EX Emulator project is a series of emulators primarily targeting Android and Linux with a minimalist UI 
and a focus on low audio/video latency. They consist of 3 components:

1. Imagine, a platform abstraction library
2. EmuFramework, a common application framework library
3. The emulator applications themselves

## Requirements

Android: Any 64-bit device or a 32-bit device with at least Android 2.3 going as far back as the Xperia Play  
Linux: A desktop with OpenGL 3.3 support or the Pandora handheld

Saturn.emu requires a 64-bit device.

## Building

See doc/INSTALL in Imagine and EmuFramework

## Android builds

### Google Play

The latest stable builds are available on the Play Store via my [developer page](https://play.google.com/store/apps/collection/cluster?gsr=SktqGFp2dmEzNmZEOXhIaXV4b2ZkYXBaTHc9PbICKwoOCgpjb20uUGNlRW11EAcSFwgCEhM1MDUxMDg2NTA4NjQ2MzQ4OTg2GACwEgA%3D:S:ANO1ljKaDG4&hl) 
and directly help fund development.

### Nightly builds

Latest builds from continuous integration:

| Name                    | Status                            | File                                       |
|-------------------------|-----------------------------------|--------------------------------------------|
| EX emulators            | [![Build Status][Build]][Actions] | [![Emulator][Download]][EX emulators]      |
| 2600 emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][2600 emu]          |
| C64 emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][C64 emu]           |
| GBA emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][GBA emu]           |
| GBC emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][GBC emu]           |
| Lynx emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][Lynx emu]          |
| MD emu                  | [![Build Status][Build]][Actions] | [![Emulator][Download]][MD emu]            |
| MSX emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][MSX emu]           |
| NEO emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NEO emu]           |
| NES emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NES emu]           |
| NGP emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NGP emu]           |
| PCE emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][PCE emu]           |
| Saturn emu              | [![Build Status][Build]][Actions] | [![Emulator][Download]][Saturn emu]        |
| Snes9x EX 1.43          | [![Build Status][Build]][Actions] | [![Emulator][Download]][Snes9x EX 1.43  ]  |
| Snes9x EX Plus          | [![Build Status][Build]][Actions] | [![Emulator][Download]][Snes9x EX Plus]    |
| Swan emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][Swan emu]          |

[Actions]: https://github.com/Rakashazi/emu-ex-plus-alpha/actions/workflows/build.yml
[Build]: https://github.com/Rakashazi/emu-ex-plus-alpha/actions/workflows/build.yml/badge.svg
[Download]: https://img.shields.io/badge/Download-blue
[EX emulators]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/EX-Emulators.zip
[2600 emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/2600Emu.zip
[C64 emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/C64Emu.zip
[GBA emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/GbaEmu.zip
[GBC emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/GbcEmu.zip
[Lynx emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/LynxEmu.zip
[MD emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/MdEmu.zip
[MSX emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/MsxEmu.zip
[NEO emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NeoEmu.zip
[NES emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NesEmu.zip
[NGP emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NgpEmu.zip
[PCE emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/PceEmu.zip
[Saturn emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/SaturnEmu.zip
[Snes9x EX 1.43]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/Snes9xEX.zip
[Snes9x EX Plus]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/Snes9xEXPlus.zip
[Swan emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/SwanEmu.zip
