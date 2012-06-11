ENV := android

android_ndkSysroot := $(ANDROID_NDK_PATH)/platforms/android-$(android_minSDK)/arch-$(android_ndkArch)

android_hasSDK9 := $(shell expr $(android_minSDK) \>= 9)

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/android-$(android_minSDK)/libs-release/$(android_abi)
 else
  targetDir := target/android-$(android_minSDK)/libs-debug/$(android_abi)
 endif
endif

ifeq ($(android_hasSDK9), 1)
 android_soName := main
else
 android_soName := imagine
endif

#CPPFLAGS += -isystem $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/include
ifndef android_stdcxx
 ifdef cxxExceptions
  android_stdcxx := gnu
 else
  android_stdcxx := stlport
 endif
endif

ifeq ($(android_stdcxx), gnu)
 android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/$(android_abi)/libgnustl_static.a
else
 android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/$(android_abi)/libstlport_static.a -lstdc++
endif

include $(currPath)/gcc.mk

#BASE_CXXFLAGS += -fno-use-cxa-atexit
# -fshort-wchar -funwind-tables -fstack-protector -finline-limit=64 -fno-strict-aliasing
COMPILE_FLAGS += -fsingle-precision-constant -ffunction-sections -fdata-sections \
-Wa,--noexecstack $(android_cpuFlags)
ASMFLAGS += $(android_cpuFlags)
LDFLAGS += $(android_cpuFlags)
WARNINGS_CFLAGS += -Wno-psabi -Wdouble-promotion
LDFLAGS += -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-soname,lib$(android_soName).so -shared #-Wl,-rpath-link=$(android_ndkSysroot)/usr/lib
LDLIBS += -L$(android_ndkSysroot)/usr/lib -lc -lm #-lgcc

CPPFLAGS += -DANDROID --sysroot=$(android_ndkSysroot)

#CPPFLAGS += -isystem $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/include

LDFLAGS += -s -Wl,-O1,--gc-sections,--sort-common
OPTIMIZE_LDFLAGS +=
noDoubleFloat=1