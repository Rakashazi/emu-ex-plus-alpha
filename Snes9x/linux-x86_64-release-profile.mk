PROFILE := 1
O_RELEASE := 1
LTO_MODE := lto
-include config.mk
include $(IMAGINE_PATH)/make/linux-x86_64-gcc.mk
include build.mk
