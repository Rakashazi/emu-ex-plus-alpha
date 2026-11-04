#!/bin/bash
CUSTOM_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export IMAGINE_PATH="$CUSTOM_ROOT/imagine"
export EMUFRAMEWORK_PATH="$CUSTOM_ROOT/EmuFramework"
export IMAGINE_SDK_PATH="$CUSTOM_ROOT/custom/imagine-sdk"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}"
export ANDROID_HOME="$ANDROID_SDK_ROOT"
export ANDROID_NDK_PATH="${ANDROID_NDK_PATH:-$CUSTOM_ROOT/custom/toolchain/android-ndk-r30-beta1}"
export ANDROID_CLANG_TOOLCHAIN_PATH="$ANDROID_NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64"
: "${JAVA_HOME:?Set JAVA_HOME to your JDK 17 directory}"
export JAVA_HOME
export PATH="$CUSTOM_ROOT/custom/toolchain/cmake-4.3.0-linux-x86_64/bin:$JAVA_HOME/bin:$PATH"
