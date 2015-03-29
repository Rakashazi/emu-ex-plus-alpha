include $(IMAGINE_PATH)/make/config.mk
O_RELEASE := 1
O_LTO := 1
android_minSDK := 9
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/android-arm64.mk
include $(projectPath)/build.mk
