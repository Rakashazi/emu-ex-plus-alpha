webos_osVersion := 1
O_RELEASE := 1
O_LTO := 1
-include config.mk
include $(IMAGINE_PATH)/make/webos-armv7-gcc.mk
include build.mk
