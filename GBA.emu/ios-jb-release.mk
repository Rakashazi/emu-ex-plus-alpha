metadata_confDeps := ../EmuFramework/metadata/conf.mk
ios_arch := armv7
HIGH_OPTIMIZE_CFLAGS = -Ofast -ffast-math -fmerge-all-constants -fomit-frame-pointer -fno-vectorize
include $(IMAGINE_PATH)/make/shortcut/meta-builds/ios-release.mk
