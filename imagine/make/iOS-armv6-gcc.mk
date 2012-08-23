include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))config.mk
include $(currPath)/iOS-gcc.mk
ARCH := arm
SUBARCH := armv7

ifndef targetSuffix
 targetSuffix := -armv6
endif

CHOST := arm-apple-darwin11
IOS_FLAGS += -arch armv6
ASMFLAGS += -arch armv6
system_externalSysroot := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
CPPFLAGS += -I$(system_externalSysroot)/include
LDLIBS += -L$(system_externalSysroot)/lib
# avoid complaints about armv7 libclang_rt.ios.a
LDFLAGS += -Wl,-allow_sub_type_mismatches