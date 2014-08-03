ENV := android
CROSS_COMPILE := 1
configDefs += CONFIG_MACHINE_$(MACHINE)
binStatic := 1
android_libm ?= -lm

android_hasSDK14 := $(shell expr $(android_minSDK) \>= 14)
android_hasSDK16 := $(shell expr $(android_minSDK) \>= 16)

ifeq ($(android_hasSDK16), 1)
 android_ndkSDK := 16
else ifeq ($(android_hasSDK14), 1)
 android_ndkSDK := 14
else
 android_ndkSDK := 9
endif

# TODO: android_minSDK should only apply to APK metadata
ifdef android_minLibSDK
 android_ndkSysroot := $(ANDROID_NDK_PATH)/platforms/android-$(android_minLibSDK)/arch-$(android_ndkArch)
else
 android_ndkSysroot := $(ANDROID_NDK_PATH)/platforms/android-$(android_ndkSDK)/arch-$(android_ndkArch)
endif

VPATH += $(android_ndkSysroot)/usr/lib

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/android-$(android_minSDK)/libs-release/$(android_abi)
 else
  targetDir := target/android-$(android_minSDK)/libs-debug/$(android_abi)
 endif
endif

android_soName := main

compiler_noSanitizeAddress := 1
ifeq ($(config_compiler),clang)
 # TODO: not 100% working yet
 ifeq ($(origin CC), default)
  CC := clang
  CXX := $(CC)
 endif
 include $(buildSysPath)/clang.mk
else
 include $(buildSysPath)/gcc.mk
endif

ifndef android_stdcxx
 android_stdcxx := gnu
endif

ifeq ($(android_stdcxx), gnu)
 android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)$(android_hardFPExt)/libgnustl_static.a
 ifeq ($(ARCH), arm)
  ifeq ($(android_armState),-mthumb)
   android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)$(android_hardFPExt)/thumb/libgnustl_static.a
  endif	
 endif
else
 # TODO: libc++
endif

pkg_stdcxxStaticLib := $(android_stdcxxLib)

ifdef ANDROID_APK_SIGNATURE_HASH
 CPPFLAGS += -DANDROID_APK_SIGNATURE_HASH=$(ANDROID_APK_SIGNATURE_HASH)
endif

COMPILE_FLAGS += -ffunction-sections -fdata-sections \
-Wa,--noexecstack $(android_cpuFlags) -no-canonical-prefixes
ASMFLAGS += -Wa,--noexecstack $(android_cpuFlags)
LDFLAGS += $(android_cpuFlags) --sysroot=$(android_ndkSysroot) -no-canonical-prefixes \
-Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now
LDFLAGS_SO := -Wl,-soname,lib$(android_soName).so -shared
LDLIBS += -lgcc -lc $(android_libm)
CPPFLAGS += -DANDROID --sysroot=$(android_ndkSysroot)
LDFLAGS += -s -Wl,-O1,--gc-sections,--compress-debug-sections=zlib,--icf=all,--as-needed