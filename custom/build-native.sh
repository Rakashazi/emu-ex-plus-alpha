#!/bin/bash
set -euo pipefail
source "$(dirname "$(readlink -f "$0")")/env.sh"
mkdir -p "$CUSTOM_ROOT/custom/build-logs"
cd "$IMAGINE_PATH/bundle/all"
bash makeAll-android-arm64.sh install -j8 'tempDir=/tmp/snes-custom-ndk30/$(pkgName)'
for project in "$IMAGINE_PATH" "$EMUFRAMEWORK_PATH"; do
 cd "$project"
 cmake --preset android-arm64
 cmake --build build/android-arm64 --config Release --target install -j8
done
cd "$CUSTOM_ROOT/Snes9x"
cmake --preset android-arm64 -DSNES9X_ANDROID_BRIDGE=ON
cmake --build build/android-arm64 --config Release -j8
cd "$CUSTOM_ROOT"
python3 custom/package.py emulator
