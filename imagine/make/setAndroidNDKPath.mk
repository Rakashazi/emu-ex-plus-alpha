ifneq ($(ANDROID_SDK_ROOT),)
 studioNDKBasePath := $(ANDROID_SDK_ROOT)
else ifneq ($(ANDROID_HOME),)
 studioNDKBasePath := $(ANDROID_HOME)
endif

ifneq ($(studioNDKBasePath),)
 # check for side-by-side NDK
 ifneq ($(wildcard $(studioNDKBasePath)/ndk)),)
  # choose the newest NDK version
  studioNDKPath := $(lastword $(sort $(wildcard $(studioNDKBasePath)/ndk/*)))
 endif
endif

ANDROID_NDK_PATH ?= $(studioNDKPath)

ifeq ($(wildcard $(ANDROID_NDK_PATH)/sysroot/usr/include/android),)
 $(error Can't find Android NDK, please set ANDROID_SDK_ROOT or ANDROID_HOME to your SDK root path, or set ANDROID_NDK_PATH to your NDK root path)
endif

ifdef V
 $(info Android NDK path: $(ANDROID_NDK_PATH))
endif
