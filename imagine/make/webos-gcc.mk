ENV := webos
ifeq ($(origin CC), default)
	CC := arm-none-linux-gnueabi-gcc
endif

ifndef targetDir
 ifdef O_RELEASE
  targetDir := target/webOS/bin-release
 else
  targetDir := target/webOS/bin-debug
 endif
endif

configDefs += CONFIG_ENV_WEBOS

ifndef WEBOS_PDK_PATH
 ifeq ($(webos_osVersion), 3)
  WEBOS_PDK_PATH := /opt/PalmPDK/3
 else
  WEBOS_PDK_PATH := /opt/PalmPDK/1
 endif
endif

include $(currPath)/gcc.mk

ifeq ($(webos_osVersion), 3)
 CPPFLAGS += -DCONFIG_ENV_WEBOS_OS=3
else
 CPPFLAGS += -DCONFIG_ENV_WEBOS_OS=1
endif

COMPILE_FLAGS += $(webos_cpuFlags) -fsingle-precision-constant -ffunction-sections -fdata-sections
CPPFLAGS += -I$(WEBOS_PDK_PATH)/include -I$(WEBOS_PDK_PATH)/include/SDL -include $(WEBOS_PDK_PATH)/include/glibc24symbols.c
WARNINGS_CFLAGS += -Wdouble-promotion -Wno-psabi

LDLIBS += -L$(WEBOS_PDK_PATH)/device/lib -Wl,--allow-shlib-undefined

OPTIMIZE_LDFLAGS += 
LDFLAGS += -s -Wl,-O1,--gc-sections,--as-needed,--hash-style=gnu,--sort-common
ASMFLAGS += $(webos_cpuFlags)

noDoubleFloat=1

ifdef O_LTO
 # can't use _FORTIFY_SOURCE with LTO else GLIBC symbols > 2.4 get linked in due to .symver not working
 # use _GNU_SOURCE to avoid 2.7 sscanf symbol
 COMPILE_FLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_GNU_SOURCE #-flto-partition=none
endif
