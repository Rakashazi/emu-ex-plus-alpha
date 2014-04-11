include $(IMAGINE_PATH)/make/config.mk

SUBARCH := armv7
webos_cpuFlags := -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
#-mthumb #-march=armv7-a

include $(buildSysPath)/webos-gcc.mk

ifndef target
 ifeq ($(webos_osVersion), 3)
  target := 3-armv7
 else
  target := armv7
 endif
endif

openGLESVersion ?= 2