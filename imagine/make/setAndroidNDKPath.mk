defaultNDKPath := $(HOME)/android-ndk
ANDROID_NDK_PATH ?= $(defaultNDKPath)

ifeq ($(wildcard $(ANDROID_NDK_PATH)/platforms)),)
 $(error Invalid NDK path:$(ANDROID_NDK_PATH), define ANDROID_NDK_PATH or move NDK to the default path:$(defaultNDKPath)
endif

ifdef V
 $(info using NDK path: $(ANDROID_NDK_PATH))
endif
