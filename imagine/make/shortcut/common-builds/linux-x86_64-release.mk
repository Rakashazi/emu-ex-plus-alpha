O_RELEASE := 1
O_LTO := 1
-include config.mk
include $(IMAGINE_PATH)/make/linux-x86_64-gcc.mk
include build.mk
