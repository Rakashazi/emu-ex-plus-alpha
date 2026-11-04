# Security scope — 2026-11-04

This assessment concerns the added bridge and custom packaging. It is not an audit of every upstream emulator, ROM/archive parser, device OS or dependency, and does not guarantee zero vulnerabilities. No combined binary is distributed while the upstream licensing question remains unresolved.

## Reproduced findings and treatment

- The earlier custom package explicitly enabled debugging and used a legacy development signer. Locally readable copies of that old key had existed on a filesystem that did not enforce private mode bits; there is no evidence of another user's access, but prior exposure cannot be ruled out. New local Release builds disable debugging/backups/cleartext and use a fresh private RSA-3072 identity on a filesystem enforcing 0700/0600. The obsolete workspace key link is removed; no legacy signer is used for new builds. Existing apps are not silently replaced or migrated.
- A controller lease could be renewed after its documented expiry when the main looper delayed the watchdog. This was reproduced with actual Binder and the production Java service in an isolated fixed-data fixture. Expiry is now checked synchronously on controller access/acquisition as well as by the watchdog. The native frame path independently expires queued input.
- Oversized requests, trailing data and one-way commands were previously accepted. No permission bypass or data theft was demonstrated. The synchronous v1 interface now bounds bytes before Bundle decoding, checks known field types, rejects trailing data and does not execute one-way commands. The largest valid sequence and bounded unknown extension fields remain supported.
- The initial per-source stack-protector setting did not reach the compile command because the target is defined in the parent CMake directory. TARGET_DIRECTORY corrects this pre-publication build finding; the generated command and resulting library are checked.

## Preserved boundaries

The exported bound service and every request require user-granted READ. Control additionally requires CONTROL, an exclusive owner UID/Binder and an unexpired lease. READ intentionally includes ROM and WRAM; grant it only to trusted apps. This is not a file, arbitrary host-memory or write API. Native ranges, masks, sequence sizes and durations are bounded. Reads return mutex-protected copies with generation/frame metadata. Manual digital input cancels automation; lifecycle changes discard queued input.

The bridge defaults OFF in upstream CMake; custom builds opt in. Its JNI translation unit overrides upstream's optimized stack-protector disabling. Existing Android linker RELRO, eager binding and a non-executable stack are preserved and checked. This does not claim equivalent compiler hardening throughout upstream libraries.

## Verification and limits

Host ASan/UBSan tests pass. Isolated Android 16 tests verify denied READ, READ-only/CONTROL separation, malformed requests, compatible extension fields, maximum sequence and expiry under a delayed watchdog. See [record](SECURITY-CHECKS.txt). Fixed-data tests prove Java authorization/protocol behavior, not real-game timing. Earlier debug-build tests exercised actual controller sampling, manual interruption and tracking. The newly signed Release needs deliberate live-game/migration acceptance; no owner app or game state was replaced for these checks.

The local Release APK is checked for flags, permissions, exact native library, JNI exports, signature and absence of keys/test fixtures/games. Private signing material, ROMs, saves, WRAM captures, workstation logs and private documents are excluded from the contribution. Public fingerprints are compatibility metadata, not secrets.

Remaining risks include a compromised/rooted OS or signing workstation, platform Bundle/Binder/JNI parsing, upstream native parsers, authorized-client resource pressure and Android lifecycle variability. Request bounds cannot remove platform transport allocations. A trusted READ client intentionally obtains exposed game memory/ROM. A tracker pinned to the old DEV certificate will reject the new signer until a coordinated trust update; do not disable verification to bypass this. Previously obtained debug APKs cannot be recalled.

Report new suspected issues privately through the maintainer's established channel where available. This proposal claims no critical upstream exploit and contains no private keys, user-data captures or weaponized exploit payloads.
