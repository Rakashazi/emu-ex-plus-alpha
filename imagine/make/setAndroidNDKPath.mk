ifneq ($(ANDROID_SDK_ROOT),)
 studioNDKPath := $(ANDROID_SDK_ROOT)/ndk-bundle
else ifneq ($(ANDROID_HOME),)
 studioNDKPath := $(ANDROID_HOME)/ndk-bundle
endif

ANDROID_NDK_PATH ?= $(studioNDKPath)

ifeq ($(wildcard $(ANDROID_NDK_PATH)/sysroot/usr/include/android),)
 $(error Can't find Android NDK, please set ANDROID_SDK_ROOT or ANDROID_HOME to your SDK root path, or set ANDROID_NDK_PATH to your NDK root path)
endif

ifdef V
 $(info Android NDK path: $(ANDROID_NDK_PATH))
endif
