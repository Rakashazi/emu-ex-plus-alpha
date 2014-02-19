include $(IMAGINE_PATH)/make/config.mk
config_ios_jb := 1
O_RELEASE := 1
O_LTO := 1
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/iOS-armv7s-gcc.mk
include $(projectPath)/build.mk
