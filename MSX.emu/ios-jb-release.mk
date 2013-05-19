metadata_confDeps := ../EmuFramework/metadata/conf.mk
ios_makefileOpts := O_RELEASE=1
# crashes on some file IO operations with LTO, cause unknown
#ios_makefileOpts += O_LTO=1
include $(IMAGINE_PATH)/make/shortcut/meta-builds/ios-release.mk
