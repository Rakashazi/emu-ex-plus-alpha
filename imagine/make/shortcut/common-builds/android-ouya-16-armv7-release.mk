include $(IMAGINE_PATH)/make/config.mk
O_RELEASE := 1
O_LTO := 1
android_minSDK := 16
libNameExt := -ouya
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/android-ouya-armv7.mk
include $(projectPath)/build.mk
