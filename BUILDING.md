# Building emu-ex-plus-alpha

This guide provides comprehensive instructions for building the emu-ex-plus-alpha emulator suite across multiple platforms.

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Environment Setup](#environment-setup)
- [Building for Linux](#building-for-linux)
- [Building for Android](#building-for-android)
- [Building for iOS](#building-for-ios)
- [Building for macOS](#building-for-macos)
- [Building for Pandora](#building-for-pandora)
- [Build System Details](#build-system-details)
- [Troubleshooting](#troubleshooting)

---

## Overview

The emu-ex-plus-alpha project is a unified emulator suite built on two core frameworks:

- **Imagine Engine**: Cross-platform application framework providing graphics, input, audio, and windowing support
- **EmuFramework**: Common emulator frontend framework providing shared infrastructure (save states, configuration, UI menus, input mapping)

Each `*.emu` folder contains an emulator that wraps a specific upstream emulator core (Stella, VICE, VBA-M, Gambatte, etc.) with this unified frontend.

---

## Prerequisites

### Required Tools (All Platforms)

#### C++ Compiler
One of the following with **C++20 support**:
- **GCC**: Version 16 or later
- **Clang**: Version 21 or later
- **MSVC**: Visual Studio 2019 or later (for Windows builds)

#### Build Tools
- **CMake**: Version 4.1 or later (primary build system)
- **GNU Make**: For Makefile-based builds
- **pkg-config**: For dependency management
- **Git**: For cloning the repository

### Platform-Specific Requirements

#### Linux
- **Development Libraries**:
  - X11 or Wayland development headers
  - OpenGL/Mesa development libraries
  - ALSA or PulseAudio development headers
- **Linker**: mold (recommended) or ld

Example installation (Ubuntu/Debian):
```bash
sudo apt-get install build-essential cmake pkg-config mold git \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libgl1-mesa-dev libasound2-dev
```

Example installation (Fedora/RHEL):
```bash
sudo dnf install gcc-c++ cmake pkgconfig mold git \
  libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel \
  mesa-libGL-devel alsa-lib-devel
```

#### Android
- **Android NDK**: Latest version recommended
- **Android SDK**: API level 21 (Android 5.0) or higher
- **Java Development Kit (JDK)**: JDK 11 or later

#### iOS
- **Xcode**: Latest version with Command Line Tools
- **iOS SDK**: Included with Xcode
- **MacPorts** (optional but recommended):
  ```bash
  sudo port install coreutils libtool pkgconfig gnutar
  ```

#### macOS
- **Xcode**: Latest version with Command Line Tools
- **Homebrew** (recommended for dependencies):
  ```bash
  brew install cmake pkg-config
  ```

#### Pandora
- **Pandora SDK**: OpenPandora development environment
- **ARM cross-compilation toolchain**

---

## Environment Setup

### 1. Clone the Repository

```bash
git clone https://github.com/Rakashazi/emu-ex-plus-alpha.git
cd emu-ex-plus-alpha
```

### 2. Set Environment Variables

The build system requires environment variables to locate the Imagine framework and SDK:

```bash
# Set the path to the Imagine framework (required)
export IMAGINE_PATH=/path/to/emu-ex-plus-alpha/imagine

# Set the SDK installation path (optional, defaults to $HOME/imagine-sdk)
export IMAGINE_SDK_PATH=$HOME/imagine-sdk
```

For permanent setup, add these to your shell configuration file (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
echo 'export IMAGINE_PATH=/path/to/emu-ex-plus-alpha/imagine' >> ~/.bashrc
echo 'export IMAGINE_SDK_PATH=$HOME/imagine-sdk' >> ~/.bashrc
source ~/.bashrc
```

---

## Building for Linux

### Step 1: Build the Imagine SDK

The Imagine SDK must be built first before building any emulators:

```bash
cd $IMAGINE_PATH
cmake --preset linux-x86_64
cmake --build build/linux-x86_64 --target install
```

This installs the Imagine SDK to `$IMAGINE_SDK_PATH`.

### Step 2: Build an Emulator

Navigate to the emulator you want to build and run:

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
cmake --preset linux-x86_64
cmake --build build/linux-x86_64
```

Replace `[EMULATOR]` with the desired emulator (e.g., `NES`, `GBA`, `C64`, etc.).

### Step 3: Install (Optional)

```bash
cmake --build build/linux-x86_64 --target install
```

### Alternative: Using Legacy Makefiles

Each emulator includes a `linux.mk` makefile for legacy builds:

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
make -f linux.mk
```

---

## Building for Android

### Step 1: Install Android NDK and SDK

1. Download and install [Android Studio](https://developer.android.com/studio)
2. Install NDK via SDK Manager:
   - Open Android Studio → Settings → Appearance & Behavior → System Settings → Android SDK
   - Navigate to SDK Tools tab
   - Check "NDK (Side by side)" and click Apply

3. Set environment variables:
```bash
export ANDROID_HOME=$HOME/Android/Sdk
export ANDROID_NDK=$ANDROID_HOME/ndk/[VERSION]
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

### Step 2: Build Dependencies

Build all Android dependencies using the Imagine framework:

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-android.sh install
```

This process will:
- Download and build all required third-party libraries
- Install them to the Imagine SDK path
- Prepare the build environment for Android

### Step 3: Build an Emulator

Navigate to the emulator directory and build for Android:

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
make -f android.mk
```

Or use the Android-specific build script if available.

### Step 4: Generate APK

The build process will generate an APK file in the `build/` directory that can be installed on Android devices.

**Minimum Requirements:**
- API Level: 21 (Android 5.0)
- Architectures: arm64-v8a, x86_64 (64-bit required for some emulators like Saturn.emu)

---

## Building for iOS

### Step 1: Install Xcode

Install Xcode from the Mac App Store, then install Command Line Tools:

```bash
xcode-select --install
```

### Step 2: Install MacPorts Dependencies

```bash
sudo port install coreutils libtool pkgconfig gnutar
```

### Step 3: Build Dependencies

Build iOS dependencies using the Imagine framework:

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-ios.sh install
```

### Step 4: Build an Emulator

Navigate to the emulator directory:

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
make -f ios.mk
```

Or use Xcode:
1. Open the `.xcodeproj` file in the emulator's build directory
2. Select your target device or simulator
3. Build and run (⌘R)

**Requirements:**
- Architecture: arm64
- Deployment Target: iOS 13.0 or later (check individual emulator requirements)

---

## Building for macOS

### Step 1: Install Dependencies

```bash
brew install cmake pkg-config
```

### Step 2: Build Imagine SDK

```bash
cd $IMAGINE_PATH
cmake --preset macos-arm64  # For Apple Silicon
# OR
cmake --preset macos-x86_64  # For Intel Macs
cmake --build build/macos-[ARCH] --target install
```

### Step 3: Build an Emulator

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
cmake --preset macos-arm64  # or macos-x86_64
cmake --build build/macos-[ARCH]
```

---

## Building for Pandora

### Step 1: Set Up Pandora SDK

Install the OpenPandora development environment and cross-compilation toolchain.

### Step 2: Build Dependencies

```bash
cd $IMAGINE_PATH/bundle/all
bash makeAll-pandora.sh install
```

### Step 3: Build an Emulator

```bash
cd /path/to/emu-ex-plus-alpha/[EMULATOR].emu
make -f pandora.mk
```

---

## Build System Details

### CMake Build System (Recommended)

The project uses CMake as the primary build system with presets for different platforms.

#### Available CMake Presets

View available presets:
```bash
cmake --list-presets
```

Common presets:
- `linux-x86_64` - Linux 64-bit
- `linux-arm64` - Linux ARM 64-bit
- `android-arm64` - Android ARM 64-bit
- `ios-arm64` - iOS ARM 64-bit
- `macos-arm64` - macOS Apple Silicon
- `macos-x86_64` - macOS Intel

#### CMake Build Options

Key CMake variables:
- `CMAKE_BUILD_TYPE` - Build type (Debug, Release, RelWithDebInfo)
- `CMAKE_INSTALL_PREFIX` - Installation directory
- `IMAGINE_PATH` - Path to Imagine framework
- `IMAGINE_SDK_PATH` - Path to Imagine SDK installation

Example with custom options:
```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DIMAGINE_PATH=/path/to/imagine \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

### Makefile Build System (Legacy)

Each emulator includes platform-specific makefiles:
- `linux.mk` - Linux builds
- `android.mk` - Android builds
- `ios.mk` - iOS builds
- `pandora.mk` - Pandora builds

The Makefile system uses `config.mk` for configuration and `metadata/conf.mk` for package metadata.

### Build Targets

Common build targets:
- `all` (default) - Build the emulator
- `install` - Install the emulator
- `clean` - Clean build artifacts
- `help` - Show available targets

### Compiler Definitions

Key preprocessor definitions used across emulators:
- `HAVE_STDINT_H` - Standard integer types support
- `LSB_FIRST` - Little-endian byte order (x86, ARM)
- `EMU_EX_PLATFORM` - EmuEx platform integration enabled
- `NO_ASM` - Disable assembly optimizations (pure C/C++)
- `FRAMESKIP` - Enable frameskip support

### Directory Structure

```
emu-ex-plus-alpha/
├── imagine/              # Imagine framework
│   ├── include/
│   ├── src/
│   └── CMakeLists.txt
├── EmuFramework/         # Emulator framework
│   ├── include/
│   └── src/
├── [EMULATOR].emu/       # Individual emulators
│   ├── src/
│   │   ├── main/        # EmuEx integration layer
│   │   └── [core]/      # Upstream emulator core
│   ├── res/             # Resources (icons, etc.)
│   ├── metadata/        # Package metadata
│   ├── CMakeLists.txt   # CMake configuration
│   ├── config.mk        # Makefile configuration
│   └── *.mk             # Platform makefiles
└── bundle/              # Dependency bundles
    └── all/
        ├── makeAll-android.sh
        ├── makeAll-ios.sh
        └── ...
```

---

## Troubleshooting

### Common Build Issues

#### 1. "Cannot find imagine/..." header files

**Solution**: Ensure `IMAGINE_PATH` is set correctly and the Imagine SDK has been built:
```bash
export IMAGINE_PATH=/path/to/emu-ex-plus-alpha/imagine
cd $IMAGINE_PATH
cmake --preset linux-x86_64
cmake --build build/linux-x86_64 --target install
```

#### 2. CMake version too old

**Error**: `CMake 4.1 or higher is required`

**Solution**: Update CMake:
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake

# Or download from cmake.org
wget https://cmake.org/files/v3.28/cmake-3.28.1-linux-x86_64.sh
sudo sh cmake-3.28.1-linux-x86_64.sh --prefix=/usr/local --skip-license
```

#### 3. C++20 features not supported

**Error**: Compiler errors about C++20 features

**Solution**: Update your compiler:
```bash
# Ubuntu/Debian
sudo apt-get install gcc-13 g++-13
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Fedora/RHEL
sudo dnf install gcc-toolset-13
scl enable gcc-toolset-13 bash
```

#### 4. Android NDK not found

**Error**: Android NDK path not detected

**Solution**: Set the NDK path explicitly:
```bash
export ANDROID_NDK=$HOME/Android/Sdk/ndk/[VERSION]
export ANDROID_NDK_HOME=$ANDROID_NDK
```

#### 5. Missing dependencies on Linux

**Error**: Cannot find X11, OpenGL, or ALSA headers

**Solution**: Install development packages:
```bash
# Debian/Ubuntu
sudo apt-get install libx11-dev libgl1-mesa-dev libasound2-dev

# Fedora/RHEL
sudo dnf install libX11-devel mesa-libGL-devel alsa-lib-devel
```

#### 6. Linker errors about mold

**Error**: Cannot find linker `mold`

**Solution**: Either install mold or configure the build to use ld:
```bash
# Install mold
sudo apt-get install mold  # Debian/Ubuntu
sudo dnf install mold       # Fedora

# Or use ld instead (edit build configuration)
```

#### 7. Module compilation errors

**Error**: C++ modules not supported

**Solution**: Ensure you're using a compiler with C++20 modules support:
- GCC 11+ with `-fmodules-ts`
- Clang 16+ with modules enabled
- MSVC 2019 16.10+ with `/std:c++20`

#### 8. iOS code signing issues

**Error**: Code signing failed

**Solution**: Configure code signing in Xcode:
1. Open the Xcode project
2. Select the target
3. Go to "Signing & Capabilities"
4. Select your development team
5. Ensure "Automatically manage signing" is enabled

### Getting Help

If you encounter issues not covered here:

1. **Check the logs**: Build output often contains helpful error messages
2. **GitHub Issues**: https://github.com/Rakashazi/emu-ex-plus-alpha/issues
3. **Project Website**: https://www.explusalpha.com/
4. **Individual Emulator READMEs**: Check `[EMULATOR].emu/README.md` for emulator-specific build notes

### Clean Build

If you encounter persistent build issues, try a clean build:

```bash
# Clean CMake build
rm -rf build/
cmake --preset [PRESET]
cmake --build build/[PRESET]

# Clean Makefile build
make -f [platform].mk clean
make -f [platform].mk
```

---

## Build Configuration Reference

### Compiler Requirements

| Platform | Compiler | Minimum Version | Required Flags |
|----------|----------|-----------------|----------------|
| Linux | GCC | 16+ | `-std=c++20` |
| Linux | Clang | 21+ | `-std=c++20` |
| Android | Clang (NDK) | r21+ | `-std=c++20` |
| iOS | Clang (Xcode) | Xcode 13+ | `-std=c++20` |
| macOS | Clang (Xcode) | Xcode 13+ | `-std=c++20` |
| Windows | MSVC | 2019+ | `/std:c++20` |

### Platform SDK Requirements

| Platform | Minimum SDK/API | Recommended | Architecture |
|----------|-----------------|-------------|--------------|
| Android | API 21 (5.0) | API 33+ | arm64-v8a, x86_64 |
| iOS | iOS 13.0 | iOS 16.0+ | arm64 |
| macOS | macOS 11.0 | macOS 13.0+ | arm64, x86_64 |
| Linux | Kernel 3.10+ | Kernel 5.0+ | x86_64, arm64 |

### Build Time Estimates

Approximate build times on a modern system (Ryzen 9 / Core i9):

| Component | Debug Build | Release Build |
|-----------|-------------|---------------|
| Imagine SDK | ~5 minutes | ~8 minutes |
| Single Emulator | ~2 minutes | ~5 minutes |
| All Dependencies (Android) | ~30 minutes | ~60 minutes |
| Full Suite (all emulators) | ~30 minutes | ~90 minutes |

---

## Advanced Build Options

### Cross-Compilation

For cross-compiling to different architectures:

```bash
# ARM64 on x86_64
cmake --preset linux-arm64 -DCMAKE_TOOLCHAIN_FILE=toolchain-arm64.cmake
cmake --build build/linux-arm64
```

### Debug Builds

Enable debug symbols and assertions:

```bash
cmake --preset linux-x86_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux-x86_64
```

### Optimized Builds

Enable maximum optimizations:

```bash
cmake --preset linux-x86_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native"
cmake --build build/linux-x86_64
```

### Static Linking

Build with static libraries:

```bash
cmake --preset linux-x86_64 \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_EXE_LINKER_FLAGS="-static"
cmake --build build/linux-x86_64
```

---

**Project**: emu-ex-plus-alpha
**Author**: Robert Broglia
**Website**: https://www.explusalpha.com/
**Repository**: https://github.com/Rakashazi/emu-ex-plus-alpha
**License**: GNU General Public License v3.0
