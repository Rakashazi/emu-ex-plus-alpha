include $(IMAGINE_PATH)/make/config.mk
libNameExt := -ouya
-include $(projectPath)/config.mk
include $(IMAGINE_PATH)/make/android-ouya-armv7.mk
include $(projectPath)/build.mk
