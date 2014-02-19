include $(IMAGINE_PATH)/make/config.mk
config_ios_jb := 1
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk
include $(projectPath)/build.mk
