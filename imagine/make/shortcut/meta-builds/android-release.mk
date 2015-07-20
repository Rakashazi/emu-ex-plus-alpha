include $(IMAGINE_PATH)/make/config.mk
android_makefileOpts ?= O_RELEASE=1 O_LTO=1
include $(buildSysPath)/shortcut/meta-builds/android.mk