include $(IMAGINE_PATH)/make/config.mk
O_RELEASE := 1
O_LTO := 1
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk
include $(projectPath)/build.mk
