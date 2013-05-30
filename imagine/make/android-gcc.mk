ENV := android
CROSS_COMPILE := 1
configDefs += CONFIG_MACHINE_$(MACHINE)

android_hasSDK5 := $(shell expr $(android_minSDK) \>= 5)
android_hasSDK9 := $(shell expr $(android_minSDK) \>= 9)
android_hasSDK14 := $(shell expr $(android_minSDK) \>= 14)

ifeq ($(android_hasSDK14), 1)
 android_ndkSDK := 14
else ifeq ($(android_hasSDK9), 1)
 android_ndkSDK := 9
else ifeq ($(android_hasSDK5), 1)
 android_ndkSDK := 5
else
 android_ndkSDK := 4
endif

android_ndkSysroot := $(ANDROID_NDK_PATH)/platforms/android-$(android_ndkSDK)/arch-$(android_ndkArch)

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

compiler_noSanitizeAddress := 1
ifeq ($(config_compiler),clang)
 # TODO: not 100% working yet
 ifeq ($(origin CC), default)
  CC := clang
 endif
 include $(buildSysPath)/clang.mk
else
 include $(buildSysPath)/gcc.mk
endif

ifndef android_stdcxx
 android_stdcxx := gnu
endif

ifeq ($(android_stdcxx), gnu)
 android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)/libgnustl_static.a
 ifeq ($(ARCH), arm)
  ifeq ($(android_armState),-mthumb)
   android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/gnu-libstdc++/$(gccVersion)/libs/$(android_abi)/thumb/libgnustl_static.a
  endif	
 endif
else
 android_stdcxxLib := $(ANDROID_NDK_PATH)/sources/cxx-stl/stlport/libs/$(android_abi)/libstlport_static.a -lstdc++
endif

pkg_stdcxxStaticLib := $(android_stdcxxLib)

ifdef ANDROID_APK_SIGNATURE_HASH
 CPPFLAGS += -DANDROID_APK_SIGNATURE_HASH=$(ANDROID_APK_SIGNATURE_HASH)
endif

#BASE_CXXFLAGS += -fno-use-cxa-atexit
# -fstack-protector
COMPILE_FLAGS += -ffunction-sections -fdata-sections \
-Wa,--noexecstack $(android_cpuFlags)
ASMFLAGS += -Wa,--noexecstack $(android_cpuFlags)
LDFLAGS += $(android_cpuFlags)
WARNINGS_CFLAGS += -Wno-psabi
LDFLAGS += -Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now,-soname,lib$(android_soName).so -shared
LDLIBS += -L$(android_ndkSysroot)/usr/lib -lm

CPPFLAGS += -DANDROID --sysroot=$(android_ndkSysroot)
LDFLAGS += -s -Wl,-O1,--gc-sections,--compress-debug-sections=zlib,--icf=all
OPTIMIZE_LDFLAGS +=