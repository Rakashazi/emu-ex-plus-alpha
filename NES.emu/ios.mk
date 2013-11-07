metadata_confDeps := ../EmuFramework/metadata/conf.mk
ios_imagineLibPathARMv7 := $(IMAGINE_PATH)/lib/ios2-armv7
ios_imagineIncludePathARMv7 := $(IMAGINE_PATH)/build/ios2-armv7/gen
ios_armv7Makefile := $(IMAGINE_PATH)/make/shortcut/common-builds/ios2-armv7.mk
include $(IMAGINE_PATH)/make/shortcut/meta-builds/ios.mk
