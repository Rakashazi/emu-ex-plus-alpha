metadata_confDeps := ../EmuFramework/metadata/conf.mk
android_makefileOpts := O_RELEASE=1 O_LTO_LINK_ONLY=1
# slower with LTO, tested up to GCC 4.9.0
#android_makefileOpts += O_LTO=1
include $(IMAGINE_PATH)/make/shortcut/meta-builds/android-release-9.mk
