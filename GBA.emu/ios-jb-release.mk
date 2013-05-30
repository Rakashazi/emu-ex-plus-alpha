metadata_confDeps := ../EmuFramework/metadata/conf.mk
ios_makefileOpts := O_RELEASE=1
# slower with LTO, tested up to Apple LLVM version 4.2 (clang-425.0.28)
#ios_makefileOpts += O_LTO=1
ios_arch := armv7
include $(IMAGINE_PATH)/make/shortcut/meta-builds/ios-release.mk
