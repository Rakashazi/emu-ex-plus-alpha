# general Android ARM setup, included by ARM/ARMv7 makefiles
ifndef android_minSDK 
 android_minSDK := 9
endif
android_ndkArch := arm
ARCH := arm

ifneq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := arm-linux-androideabi-gcc
  CXX := $(CC)
  CHOST := arm-linux-androideabi
 endif
endif

COMPILE_FLAGS += -fpic

include $(buildSysPath)/android-gcc.mk
