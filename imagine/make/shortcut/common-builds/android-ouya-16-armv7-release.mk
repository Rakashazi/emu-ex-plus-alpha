O_RELEASE := 1
O_LTO := 1
android_minSDK := 16
-include config.mk
include $(IMAGINE_PATH)/make/android-ouya-armv7.mk
include build.mk
