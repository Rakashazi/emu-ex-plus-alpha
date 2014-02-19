# general Android ARM setup, included by ARMv6/ARMv7 makefiles
ifndef android_minSDK 
 android_minSDK := 9
endif
android_ndkArch := arm
ARCH := arm

ifneq ($(config_compiler),clang)
 ifeq ($(origin CC), default)
  CC := arm-linux-androideabi-gcc
  CHOST := arm-linux-androideabi
 endif
endif

COMPILE_FLAGS += -fpic
ifneq ($(config_compiler),clang)
 COMPILE_FLAGS += -fsingle-precision-constant
 WARNINGS_CFLAGS += -Wdouble-promotion
endif
noDoubleFloat=1

include $(buildSysPath)/android-gcc.mk
