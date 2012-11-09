all : linux-x86_64 #android-armv7 #webos-armv7

metadata_confDeps := ../EmuFramework/metadata/conf.mk

linux-x86_64 : linux-x86_64.mk
	$(MAKE) -f $<

android_noArmv6 := 1
ios_noArmv6 := 1
webos_noArmv6 := 1

include $(IMAGINE_PATH)/make/shortcut/webos.mk

include $(IMAGINE_PATH)/make/shortcut/android.mk

include $(IMAGINE_PATH)/make/shortcut/iOS.mk