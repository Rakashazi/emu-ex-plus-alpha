# Local builds and checks

This source is for upstream review. Modified combined APKs must not be published without resolving the licensing conditions described in [README](../README.md).

## Toolchain and packaging

Tested: API/Build Tools 36.0.0, NDK r30-beta1 (`30.0.14904198-beta1`), CMake 4.3.0, JDK 17, ARM64. CMake's experimental import-std identifier must match upstream. Configure `JAVA_HOME` and `ANDROID_SDK_ROOT`. `ANDROID_NDK_PATH` can override `custom/toolchain/android-ndk-r30-beta1`. Native dependencies install into local `custom/imagine-sdk`; no global SDK/NDK is modified.

Java dependencies are Android Support 28.0.0 and Architecture Components 1.1.1 from Google's Android Maven repository. See DEPENDENCIES.json and `custom/licenses`. Place required JAR/AAR files in ignored `custom/vendor`. Native dependency source/build recipes are provided under `imagine/bundle`. Toolchains, downloads, generated code, APKs and signing material are ignored.

```
python3 custom/init-signing.py       # once, protected local Release identity
bash custom/build-native.sh          # opt-in bridge, ARM64 Release, local APK only
```

Output: `custom/artifacts/snes9x-custom-release.apk`. Packaging uses a Release core, disables debugging/backups/cleartext traffic and verifies the expected signer. No network permission is added. No default debug key or embedded password is used. Private keys/passwords stay under `~/.local/share/snes9x-custom-signing/` (0700 directory, 0600 files); `SNES_PRIVATE_SIGNING_DIR` can select another protected filesystem. Keep an offline backup. Public certificates are not private keys. A clone cannot regenerate an established identity: restore its private signer or plan a separate identity explicitly. QA uses a separate signer initialized with `custom/init-signing.py --qa`.

## Native host regression

```
clang++ -std=c++20 -Wall -Wextra -Werror -pthread -fsanitize=address,undefined Snes9x/tests/bridge_test.cc -o /tmp/snes-bridge-test
ASAN_OPTIONS=detect_leaks=0 /tmp/snes-bridge-test
```

LeakSanitizer alone is disabled because the test environment's ptrace restriction prevents startup; ASan/UBSan remain active. Checks cover ranges, generations, manual priority, bounded sequences, lease expiry and concurrent coherent reads.

## Isolated Android Binder security tests

```
python3 custom/init-signing.py --qa
python3 custom/securitytest/build.py
adb install custom/artifacts/bridge-securitytest-qa.apk
```

The real emulator must already define READ/CONTROL. This fixture uses the **production BridgeService Java source** with a fixed-data NativeBridge replacement in a separate process, without an emulator core or game. It launches no Activity and only binds to itself.

Run `org.snes9x.bridge.securitytest/org.snes9x.bridge.SecurityChecks` with `am instrument -w`: first `-e suite unauthorized` with READ/CONTROL ungranted; then grant READ only and run `-e suite readonly`; finally grant CONTROL to this isolated package and run the default suite. This checks real Parcel/Binder permission separation, size/type/trailing-data/one-way rejection, the maximum legitimate sequence and expiry while its fixture main looper is delayed. Uninstall `org.snes9x.bridge.securitytest` afterwards, including private data. Do not publish QA APKs.

The optional `custom/testclient` checks actual core/input timing only with explicit authorization: build via `python3 custom/package.py testclient` and generate the original input fixture via `python3 custom/make-test-rom.py`. These local tools are never part of the production APK. They must not replace a user's active game/saves. Earlier debug-build game tests do not constitute fresh Release runtime acceptance.

## Narrow upstream compile fixes

- `imagine/src/base/android/surfaceTexture.cc`: `export namespace IG` becomes `namespace IG` in an implementation module whose declarations are already exported.
- `imagine/src/base/android/Application.cc`: one `static constexpr JNINativeMethod` table becomes `static const`; its function-pointer casts are not constant expressions for the tested Clang. Entries are unchanged.

Both followed reproduced compiler failures. No unrelated upstream behavior was repaired. Current checks and limits are recorded in SECURITY.md.
