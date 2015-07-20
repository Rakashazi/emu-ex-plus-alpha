# general Android ARM setup, included by ARM/ARMv7 makefiles 
ARCH := arm
CHOST := arm-linux-androideabi
android_ndkSDK ?= 9
android_ndkArch := arm

include $(buildSysPath)/android-gcc.mk
CFLAGS_CODEGEN += -fpic