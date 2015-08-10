defaultNDKPath := $(HOME)/android-ndk

ANDROID_NDK_PATH ?= $(shell dirname `which ndk-build`)

ifeq ($(ANDROID_NDK_PATH),)
 ANDROID_NDK_PATH := $(defaultNDKPath)
endif

ifdef V
 $(info Android NDK path: $(ANDROID_NDK_PATH))
endif
