# Experimental Snes9x EX+ Android bridge proposal

Based on **[Rakashazi/emu-ex-plus-alpha](https://github.com/Rakashazi/emu-ex-plus-alpha)** at commit `1c12fac5ce49badaadff2e2f210dcc30b89f4943`. Imagine, EmuFramework, Snes9x EX+ and the Snes9x core are the work of their original authors. This proposal adds a local, game-independent Android interface; it is not an official upstream release.

The bridge supplies coherent WRAM snapshots and a copy of the normalized loaded ROM to separately authorized local apps. A separate CONTROL permission protects bounded, frame-counted joypad sequences. Manual input cancels automation. There are no sockets, host pointers, writable-memory operations, bundled games or game-specific addresses.

- [Protocol and authorization](docs/IPC-v1.md)
- [Security assessment and limits](docs/SECURITY.md)
- [Local build and tests](docs/BUILD.md)
- [Executed security checks](docs/SECURITY-CHECKS.txt)

The CMake option `SNES9X_ANDROID_BRIDGE` defaults to **OFF**. Only the explicit local custom build enables it. Standard upstream packaging is unchanged; the experimental service is packaged through `custom/package.py` rather than added to every emulator app. Two narrow C++ module/JNI table compile fixes needed by the tested toolchain are described in BUILD.md.

## Licensing and distribution

All original copyright notices and licenses remain in force. See [Snes9x/COPYING](../Snes9x/COPYING), [Snes9x core license](../Snes9x/src/snes9x/LICENSE) and [COPYING.GPL](../COPYING.GPL). The combined emulator is **not MIT-licensed**. This source proposal does not assert permission to distribute a modified combined binary: the Snes9x/GPL-combination exception in Snes9x/COPYING explicitly concerns unmodified official releases. Maintainer review and licensing clarification are required before publishing an APK. No APK, private signing key/password, ROM, save, WRAM capture or user data is included in this contribution.

Own bridge sources retain GPL-3.0-or-later notices. Android Support/Architecture dependencies retain Apache-2.0 notices and attribution under `custom/licenses`; their versions and recorded checksums are in `custom/docs/DEPENDENCIES.json`.

The local custom ID `org.snes9x.custom` is separate from the original app. Do not update/restart a running game merely to test a build. New local Release signing uses a fresh private identity: an existing DEV install and a tracker pinned to its old certificate require a deliberate, coordinated migration. Never uninstall to bypass a signature error without preserving saves/configuration. Choose **App Folder** for saves to avoid overwriting shared ROM-side saves.
