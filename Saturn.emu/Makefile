all : linux-x86 #android-armv7 #webos-armv7

metadata_confDeps := ../EmuFramework/metadata/conf.mk

linux-x86 : linux-x86.mk
	$(MAKE) -f $<

android_noArmv7 := 1
ios_noArmv6 := 1
webos_noArmv6 := 1

include $(IMAGINE_PATH)/make/shortcut/webos.mk

include $(IMAGINE_PATH)/make/shortcut/android.mk

include $(IMAGINE_PATH)/make/shortcut/iOS.mk
