metadata_confDeps := ../EmuFramework/metadata/conf.mk
android_makefileOpts := O_RELEASE=1 O_LTO_LINK_ONLY=1
# slower with LTO, tested up to GCC 4.9.0
# TODO: strong symbols will not override weak ones in
# EmuFramework when Snes9x is not compiled with LTO
# and Emuframework is, possible linker bug?
include $(IMAGINE_PATH)/make/shortcut/meta-builds/android-release-9.mk
