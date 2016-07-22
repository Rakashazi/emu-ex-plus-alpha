include $(IMAGINE_PATH)/make/config.mk
LTO_MODE ?= lto
android_makefileOpts ?= O_RELEASE=1
include $(buildSysPath)/shortcut/meta-builds/android.mk