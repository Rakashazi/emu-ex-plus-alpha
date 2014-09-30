metadata_confDeps := ../EmuFramework/metadata/conf.mk
ios_arch = armv6 armv7 arm64
ios_makefileOpts = O_RELEASE=1
include $(IMAGINE_PATH)/make/shortcut/meta-builds/ios-release.mk
