ifndef EMUFRAMEWORK_PATH
 EMUFRAMEWORK_PATH := $(lastMakefileDir)/../EmuFramework
endif
metadata_confDeps = $(EMUFRAMEWORK_PATH)/metadata/conf.mk

# A 64-bit device is needed for performance reasons
android_minSDK = 21
android_arch = arm64 x86_64
ios_arch = arm64