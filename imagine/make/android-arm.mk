# general Android ARM setup, included by ARM/ARMv7 makefiles 
ARCH := arm
CHOST := arm-linux-androideabi
android_ndkSDK ?= 9
android_ndkArch := arm
CFLAGS_CODEGEN += -fpic

include $(buildSysPath)/android-gcc.mk
