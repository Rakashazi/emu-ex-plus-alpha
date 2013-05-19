O_RELEASE := 1
O_LTO := 1
-include config.mk
include $(IMAGINE_PATH)/make/linux-armv7-pandora-gcc.mk
include build.mk
