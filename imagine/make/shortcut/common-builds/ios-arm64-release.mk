include $(IMAGINE_PATH)/make/config.mk
O_RELEASE := 1
O_LTO := 1
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/ios-arm64.mk
include $(projectPath)/build.mk
