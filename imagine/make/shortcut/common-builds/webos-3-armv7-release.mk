include $(IMAGINE_PATH)/make/config.mk
webos_osVersion := 3
O_RELEASE := 1
O_LTO := 1
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/webos-armv7-gcc.mk
include $(projectPath)/build.mk
